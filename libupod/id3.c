/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v1.1 id3.c 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "itunesdbi.h"

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <libgen.h>

#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define ID3_DEBUG 0

#include "genre.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
                       /* v2.2 v2.3 */

char *ID3_TITLE[2]   = {"TT1", "TIT1"};
char *ID3_TALT[2]    = {"TT2", "TIT2"};
char *ID3_ARTIST[2]  = {"TP1", "TPE1"};
char *ID3_ALBUM[2]   = {"TAL", "TALB"};
char *ID3_TRACK[2]   = {"TRK", "TRCK"};
char *ID3_YEAR[2]    = {"TYE", "TYER"};
char *ID3_GENRE[2]   = {"TCO", "TCON"};
char *ID3_ENCODER[2] = {"TEN", "TENC"}; 
char *ID3_COMMENT[2] = {"COM", "COMM"};
char *ID3_BPM[2]     = {"TBP", "TBPM"};
char *ID3_YEARNEW[2] = {"TYE", "TDRC"};
char *ID3_DISC[2]    = {"TPA", "TPOS"};
char *ID3_ARTWORK[2] = {"PIC", "APIC"};

static int find_id3 (int version, FILE *fh, unsigned char *tag_data, int *tag_datalen,
		     int *id3_len, int *major_version);
static void one_pass_parse_id3 (FILE *fh, unsigned char *tag_data, int tag_datalen, int version,
                                int id3v2_majorversion, tihm_t *tihm);

u_int32_t big32_2_arch32 (u_int32_t); /* defined in aac.c */

static int synchsafe_to_int (char *buf, int nbytes) {
  int id3v2_len = 0;
  int i;

  for (i = 0 ; i < nbytes ; i++) {
    id3v2_len <<= 7;
    id3v2_len += buf[i] & 0x7f;
  }

  return id3v2_len;
}

/*
  find_id3 takes in a file descriptor, a pointer to where the tag data is to be put,
  and a pointer to where the data length is to be put.

  find_id3 returns:
    0 for no id3 tags
    1 for id3v1 tag
    2 for id3v2 tag

  The file descriptor is reset to the start of the file on completion.
*/
static int find_id3 (int version, FILE *fh, unsigned char *tag_data, int *tag_datalen,
		     int *id3_len, int *major_version) {
    int head;
    char data[10];

    char id3v2_flags;
    int  id3v2_len;
    int  id3v2_extendedlen;

    fread(&head, 4, 1, fh);
    
#if BYTE_ORDER == LITTLE_ENDIAN
    head = bswap_32(head);
#endif


    if (version == 2) {
      /* version 2 */
      if ((head & 0xffffff00) == 0x49443300) {
	fread(data, 1, 10, fh);
	
	*major_version = head & 0xff;
	
	id3v2_flags = data[1];
	
	id3v2_len = *id3_len = synchsafe_to_int (&data[2], 4);

	*id3_len += 10; /* total length = id3v2len + 010 + footer (if present) */
	*id3_len += (id3v2_flags & 0x10) ? 10 : 0; /* ID3v2 footer */

	/* the 6th bit of the flag field being set indicates that an
	   extended header is present */
	if (id3v2_flags & 0x40) {
	  /* Skip extended header */
	  if (*major_version != 3)
	    id3v2_extendedlen = synchsafe_to_int (&data[6], 4);
	  else
	    id3v2_extendedlen = big32_2_arch32 (((int *)&data[6])[0]);

	  fseek(fh, 0xa + 0x4 + id3v2_extendedlen, SEEK_SET);
	  *tag_datalen = id3v2_len - 0x4 - id3v2_extendedlen;
	} else {
	  /* Skip standard header */
	  fseek(fh, 0xa, SEEK_SET);
	  *tag_datalen = id3v2_len;
	}
	
	return 2;
      }
    } else if (version == 1) {
      fseek(fh, 0, SEEK_SET);
      
      /* tag not at beginning? */
      if ((head & 0xffffff00) != 0x54414700) {
	/* maybe end */
	fseek(fh, -128, SEEK_END);
	fread(&head, 1, 4, fh);
	fseek(fh, -128, SEEK_END);
	
#if BYTE_ORDER == LITTLE_ENDIAN
	head = bswap_32(head);
#endif
      }
      
      /* version 1 */
      if ((head & 0xffffff00) == 0x54414700) {
	fread(tag_data, 1, 128, fh);
	
	return 1;
      }
    }
    
    /* no id3 found */
    return 0;
}

char *id3v1_string (signed char *unclean) {
  int i;
  static char buffer[31];

  memset (buffer, 0, 31);

  for (i = 0 ; i < 30 && unclean[i] != -1 ; i++)
    buffer[i] = unclean[i];

  for (i = strlen (buffer) - 1 ; i >= 0 && buffer[i] == ' ' ; i--)
    buffer[i] = '\0';

  return buffer;
}

static int parse_artwork (tihm_t *tihm, FILE *fh, size_t length, int id3v2_majorversion) {
  unsigned char *image_data;
  int c, cksum;

  if (tihm->image_data != NULL)
    return -1;

  length --;

  fseek (fh, -length, SEEK_CUR);

  /* Skip past type string */
  for ( ; (c = fgetc (fh)) != '\0'; length--);

  /* Seek to the start of the image data */
  if (id3v2_majorversion > 2) {
    fseek (fh, 2, SEEK_CUR);
    length -= 3;
  } else {
    fseek (fh, 1, SEEK_CUR);
    length -= 2;
  }    

  image_data = (unsigned char *)calloc (1, length);
  fread (image_data, 1, length, fh);

  cksum = crc32 (image_data, length);

  tihm->has_artwork = 1;
  tihm->artwork_id1 = cksum;

  tihm->image_data  = image_data;
  tihm->image_size  = length;

  return 0;
}

/*
  parse_id3
*/
static void one_pass_parse_id3 (FILE *fh, unsigned char *tag_data, int tag_datalen, int version,
				int id3v2_majorversion, tihm_t *tihm) {
  int i, j;
  char *slash;

  if (version == 2) {
    char *tag_temp;
    char genre_temp[4];
    char encoding[11];
    char identifier[5];
    int newv = (id3v2_majorversion > 2) ? 1 : 0;

    memset (identifier, 0, 5);

    for (i = 0 ; i < tag_datalen ; ) {
      size_t length = 0;

      fread (tag_data, 1, (id3v2_majorversion > 2) ? 10 : 6, fh);
      
      if (tag_data[0] == 0)
	return;

      memcpy (identifier, tag_data, newv ? 4 : 3);

      if (id3v2_majorversion > 2) {
	/* id3v2.3 does not use synchsafe integers in frame headers. */
	if (id3v2_majorversion == 3 || strcmp (identifier, "APIC") == 0 ||
	    strcmp (identifier, "COMM") == 0 || strcmp (identifier, "COM ") == 0)
	  length = big32_2_arch32 (((int *)tag_data)[1]);
	else
	  length = synchsafe_to_int (&tag_data[4], 4);

	i += 10 + length;
      } else {
	length = (tag_data[3] << 16) | (tag_data[4] << 8) | tag_data[5];

	i += 6 + length;
      }

      if (length < 0) {
	fprintf (stderr, "id3v2 tag data length is < 0... Aborting!\n");
	break;
      } else if (length == 0)
	continue;

      if (length < 2) {
	fseek (fh, length, SEEK_CUR);

	continue;
      }

      if (length > 128) {
	fseek (fh, length - 128, SEEK_CUR);
	if (strcmp (identifier, "APIC") != 0 &&
	    strcmp (identifier, "PIC") != 0)
	  length = 128;
      }

      memset (tag_data, 0, 128);
      fread (tag_data, 1, (length < 128) ? length : 128, fh);

      tag_temp = tag_data;

      /* Get the tag encoding */
      switch (*tag_temp) {
      case 0x00:
	sprintf (encoding, "ISO-8859-1");
	tag_temp++;
	break;
      case 0x01:
	sprintf (encoding, "UTF-16LE");
	tag_temp += 3;
	break;
      case 0x03:
	sprintf (encoding, "UTF-16BE");
	tag_temp++;
	break;
      case 0x04:
	sprintf (encoding, "UTF-8");
	tag_temp++;
	break;
      default:
	sprintf (encoding, "ISO-8859-1");
      }

      if (strcmp (identifier, "APIC") != 0 &&
	  strcmp (identifier, "PIC") != 0) {
	for ( ; length && *tag_temp == '\0' ; tag_temp++, length--);
	for ( ; length && *(tag_temp+length-1) == '\0' ; length--);
	/*      length--; */
      }

      if (length <= 0)
	continue;

      if (strcmp (identifier, ID3_TITLE[newv]) == 0 ||
	  strcmp (identifier, ID3_TALT[newv]) == 0) 
	dohm_add (tihm, tag_temp, length, encoding, IPOD_TITLE);
      else if (strcmp (identifier, ID3_ARTIST[newv]) == 0)
	dohm_add (tihm, tag_temp, length, encoding, IPOD_ARTIST);
      else if (strcmp (identifier, ID3_ALBUM[newv]) == 0)
	dohm_add (tihm, tag_temp, length, encoding, IPOD_ALBUM);
      else if (strcmp (identifier, ID3_COMMENT[newv]) == 0)
	dohm_add (tihm, tag_temp, length, encoding, IPOD_COMMENT);
      else if (strcmp (identifier, ID3_ARTWORK[newv]) == 0)
	parse_artwork (tihm, fh, length, id3v2_majorversion);
      else if (strcmp (identifier, ID3_TRACK[newv]) == 0) {
	/* some id3 tags have track/total tracks in the TRK field */
	slash = strchr (tag_temp, '/');

	if (slash) *slash = 0;

	tihm->track = strtol (tag_temp, NULL, 10);

	/* set total number of album tracks */
	if (slash)
	  tihm->album_tracks = strtol (slash+1, NULL, 10);

	if (slash) *slash = '/';
      } else if (strcmp (identifier, ID3_DISC[newv]) == 0) {
	/* some id3 tags have disk_num/total_disks in the TPOS field */
	slash = strchr (tag_temp, '/');

	if (slash) *slash = 0;

	tihm->disk_num = strtol (tag_temp, NULL, 10);

	/* set total number of album tracks */
	if (slash)
	  tihm->disk_total = strtol (slash+1, NULL, 10);
      } else if (strcmp (identifier, ID3_BPM[newv]) == 0) {
	tihm->bpm = strtol (tag_temp, NULL, 10);
      } else if (strcmp (identifier, ID3_YEARNEW[newv]) == 0 ||
	       strcmp (identifier, ID3_YEAR[newv]) == 0) {
	tihm->year = strtol (tag_temp, NULL, 10);
      } else if (strcmp (identifier, ID3_GENRE[newv]) == 0) {
	if (tag_temp[0] != '(') {
	  dohm_add (tihm, tag_temp, length, encoding, IPOD_GENRE);
	} else {
	  /* 41 is right parenthesis */
	  for (j = 0 ; (*(tag_temp + 1 + j) != 41) ; j++) {
	    genre_temp[j] = *(tag_temp + 1 + j);
	  }
	  
	  genre_temp[j] = 0;
	  
	  if (atoi (genre_temp) > 147)
	    continue;
	  
	  dohm_add (tihm, genre_table[atoi(genre_temp)],
		    strlen (genre_table[atoi(genre_temp)]), "ISO-8859-1", IPOD_GENRE);
	  
	  genre_temp[j] = 41;
	}
      }
    }

  } else if (version == 1) {
    char *tmp;

    dohm_add (tihm, tmp = id3v1_string (&tag_data[3])  , strlen (tmp), "ISO-8859-1", IPOD_TITLE);
    dohm_add (tihm, tmp = id3v1_string (&tag_data[33]) , strlen (tmp), "ISO-8859-1", IPOD_ARTIST);
    dohm_add (tihm, tmp = id3v1_string (&tag_data[63]) , strlen (tmp), "ISO-8859-1", IPOD_ALBUM);
    dohm_add (tihm, tmp = id3v1_string (&tag_data[93]) , strlen (tmp), "ISO-8859-1", IPOD_COMMENT);
    
    if ((tag_data[126] != 0xff) && (tihm->track == 0))
      tihm->track = tag_data[126];
    
    if ((signed char)tag_data[127] != -1)
      dohm_add (tihm, genre_table[tag_data[127]], strlen(genre_table[tag_data[127]]) - 1, "ISO-8859-1", IPOD_GENRE);
  }
}

int get_id3_info (FILE *fh, char *file_name, tihm_t *tihm) {
  int tag_datalen = 0, id3_len = 0;
  unsigned char tag_data[128];
  int version;
  int id3v2_majorversion;

  /* built-in id3tag reading -- id3v2, id3v1 */
  if ((version = find_id3(2, fh, tag_data, &tag_datalen, &id3_len, &id3v2_majorversion)) != 0)
    one_pass_parse_id3(fh, tag_data, tag_datalen, version, id3v2_majorversion, tihm);

  /* some mp3's have both tags so check v1 even if v2 is available */
  if ((version = find_id3(1, fh, tag_data, &tag_datalen, NULL, &id3v2_majorversion)) != 0)
    one_pass_parse_id3(fh, tag_data, tag_datalen, version, id3v2_majorversion, tihm);

  /* Set the file descriptor at the end of the id3v2 header (if one exists) */
  fseek (fh, id3_len, SEEK_SET);
  
  if (tihm->num_dohm == 0 || tihm->dohms[0].type != IPOD_TITLE) {
    char *tfile_name = strdup (file_name);
    char *tmp = (char *)basename(tfile_name);
    int i;
    
    for (i=strlen(tmp)-1; i > 0 ; i--)
      if (tmp[i] == '.') {
	tmp[i] = 0;
	break;
      }
    
    dohm_add (tihm, tmp, strlen(tmp), "ISO-8859-1", IPOD_TITLE);

    free (tfile_name);
  }
  
  return 0;
}

