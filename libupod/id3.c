/**
 *   (c) 2003-2004 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.2 id3.c 
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

#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define ID3_DEBUG 1

#include "genre.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "hexdump.c"

#define ID3_TITLE             0
#define ID3_TALT              1
#define ID3_ARTIST            2
#define ID3_ALBUM             3
#define ID3_TRACK             4
#define ID3_YEAR              5
#define ID3_GENRE             6
#define ID3_ENCODER           7
#define ID3_COMMENT           8
#define ID3_BPM               11
#define ID3_YEARNEW           12
#define ID3_DISKNUM           13

static int find_id3 (int version, FILE *fh, char *tag_data, int *tag_datalen,
		     int *id3_len, int *major_version);
static void parse_id3 (FILE *fh, char *tag_data, int tag_datalen, int version,
		       int id3v2_majorversion, int field, tihm_t *tihm);

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
static int find_id3 (int version, FILE *fh, char *tag_data, int *tag_datalen,
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
	  id3v2_extendedlen = synchsafe_to_int (&data[6], 4);
	  
	  fseek(fh, 0xa + id3v2_extendedlen, SEEK_SET);
	  *tag_datalen = id3v2_len - id3v2_extendedlen;
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

/*
  parse_id3
*/
static void one_pass_parse_id3 (FILE *fh, char *tag_data, int tag_datalen, int version,
				int id3v2_majorversion, tihm_t *tihm) {
  int data_type;
  int i, j;
  int field;
  char *slash;

  if (version == 2) {
    /* field tags associated with id3v2 with major version <= 2 */
    char *fields[]     = {"TT1", "TT2", "TP1", "TAL", "TRK", "TYE", "TCO",
			  "TEN", "COM", "TLE", "TKE", NULL};
    /* field tags associated with id3v2 with major version > 2 */
    char *fourfields[] = {"TIT1", "TIT2", "TPE1", "TALB", "TRCK", "TYER", "TCON",
			  "TENC", "COMM", "TLEN", "TIME", "TBPM", "TDRC", "TPOS",
			  NULL};
    
    char *tag_temp;
    char *sizeloc;
    char genre_temp[4];
    char encoding[11];

    for (i = 0 ; i < tag_datalen ; ) {
      int length = 0;
      int tag_found = 0;
      u_int8_t *tmp;

      fread (tag_data, 1, (id3v2_majorversion > 2) ? 10 : 6, fh);
      
      if (tag_data[0] == 0)
	return;
      
      if (id3v2_majorversion > 2) {
	if (strncmp (tag_data, "APIC", 4) == 0 || id3v2_majorversion == 4)
	  length = *((int *)&tag_data[4]);
	else
	  length = synchsafe_to_int (&tag_data[4], 4);


	for (field = 0 ; fourfields[field] != NULL ; field++)
	  if (strncmp(tag_data, fourfields[field], 4) == 0) {
	    tag_found = 1;
	    break;
	  }

	i += 10 + length;
      } else {
	if (strncmp (tag_data, "PIC", 3) == 0)
	  length = (tag_data[3] << 16) | (tag_data[4] << 8) | tag_data[5];
	else
	  length = synchsafe_to_int (&tag_data[3], 3);

	for (field = 0 ; fields[field] != NULL ; field++)
	  if (strncmp(tag_data, fields[field], 3) == 0) {
	    tag_found = 1;

	    break;
	  }

	i += 6 + length;
      }

      if (tag_found == 0 || length < 2) {
	fseek (fh, length, SEEK_CUR);

	continue;
      }

      memset (tag_data, 0, 128);
      fread (tag_data, 1, (length < 128) ? length : 128, fh);

      if (length > 128)
	fseek (fh, length - 128, SEEK_CUR);

      tag_temp = tag_data;

      /* Scan past the language field in id3v2 */
      if (field == ID3_COMMENT) {
	tag_temp += 4;
	length -= 4;

	if (id3v2_majorversion > 2) {
	  tag_temp++; length--;
	}
      }

      switch (*tag_temp) {
      case 0x00:
	sprintf (encoding, "ISO-8859-1");
	break;
      case 0x01:
	sprintf (encoding, "UTF-16");
	break;
      case 0x03:
	sprintf (encoding, "UTF-16BE");
	break;
      case 0x04:
	sprintf (encoding, "UTF-8");
	break;
      default:
	sprintf (encoding, "ISO-8859-1");
      }
      
      for ( ; length && *tag_temp == '\0' ; tag_temp++, length--);

      length--;

      if (length <= 0)
	continue;

      switch (field) {
      case ID3_TITLE:
      case ID3_TALT:
	dohm_add (tihm, tag_temp, length, encoding, IPOD_TITLE);
	break;
      case ID3_ARTIST:
	dohm_add (tihm, tag_temp, length, encoding, IPOD_ARTIST);
	break;
      case ID3_ALBUM:
	dohm_add (tihm, tag_temp, length, encoding, IPOD_ALBUM);
	break;
      case ID3_COMMENT:
	dohm_add (tihm, tag_temp, length, encoding, IPOD_COMMENT);
	break;
      case ID3_TRACK:
	/* some id3 tags have track/total tracks in the TRK field */
	slash = strchr (tag_temp, '/');

	if (slash) *slash = 0;

	tihm->track = strtol (tag_temp, NULL, 10);

	/* set total number of album tracks */
	if (slash)
	  tihm->album_tracks = strtol (slash+1, NULL, 10);

	if (slash) *slash = '/';
	break;
      case ID3_DISKNUM:
	/* some id3 tags have disk_num/total_disks in the TPOS field */
	slash = strchr (tag_temp, '/');

	if (slash) *slash = 0;

	tihm->disk_num = strtol (tag_temp, NULL, 10);

	/* set total number of album tracks */
	if (slash)
	  tihm->disk_total = strtol (slash+1, NULL, 10);

	break;
      case ID3_BPM:
	tihm->bpm = strtol (tag_temp, NULL, 10);
	break;
      case ID3_YEARNEW:
      case ID3_YEAR:
	tihm->year = strtol (tag_temp, NULL, 10);
	break;
      case ID3_GENRE:
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
	break;
      deafult:
	break;
      }
    }
    
  } else if (version == 1) {
    for (field = 0 ; field < ID3_BPM ; field++) {
      char *copy_from, *tmp;
      
      i = 29;

      switch (field) {
      case ID3_TITLE:
	copy_from = &tag_data[3];
	data_type = IPOD_TITLE;
	break;
      case ID3_ARTIST:
	copy_from = &tag_data[33];
	data_type = IPOD_ARTIST;
	break;
      case ID3_ALBUM:
	copy_from = &tag_data[63];
	data_type = IPOD_ALBUM;
	break;
      case ID3_COMMENT:
	copy_from = &tag_data[93];
	data_type = IPOD_COMMENT;
	break;
      case ID3_GENRE:
	if ((signed char)tag_data[127] == -1)
	  continue;
	
	copy_from = genre_table[tag_data[127]];
	i = strlen (copy_from - 1);
	data_type = IPOD_GENRE;
	break;
      default:
	continue;
      }
      
      if ((signed char) copy_from[0] == -1)
	continue;
      
      if (field != ID3_GENRE)
	for (tmp = copy_from + i ; (*tmp == ' ' || (signed char)(*tmp) == -1) && i >= 0; tmp--, i--)
	  *tmp = 0;
      else
	i = strlen(copy_from) - 1;
      
      if (i < 0)
	continue;
      
      i++;
      dohm_add (tihm, copy_from, i, "ISO-8859-1", data_type);
    }
  }
}

int get_id3_info (FILE *fh, char *file_name, tihm_t *tihm) {
  int tag_datalen = 0, id3_len = 0;
  char tag_data[128];
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
    char *tmp = (char *)basename(file_name);
    int i;
    
    for (i=strlen(tmp)-1; i > 0 ; i--)
      if (tmp[i] == '.') {
	tmp[i] = 0;
	break;
      }
    
    dohm_add (tihm, tmp, strlen(tmp), "ISO-8859-1", IPOD_TITLE);
  }
  
  return 0;
}

