/**
 *   (c) 2003-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v1.2 id3.c 
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

#include "itunesdbi.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#define ID3FLAG_EXTENDED 0x40
#define ID3FLAG_FOOTER   0x10
#define TAG_DATA_SIZE    256

                       /* v2.2 v2.3 */
static char *ID3_TITLE[2]   = {"TT2", "TIT2"};
static char *ID3_ARTIST[2]  = {"TP1", "TPE1"};
static char *ID3_ALBUM[2]   = {"TAL", "TALB"};
static char *ID3_TRACK[2]   = {"TRK", "TRCK"};
static char *ID3_YEAR[2]    = {"TYE", "TYER"};
static char *ID3_GENRE[2]   = {"TCO", "TCON"};
/* static char *ID3_ENCODER[2] = {"TEN", "TENC"}; */
static char *ID3_COMMENT[2] = {"COM", "COMM"};
static char *ID3_BPM[2]     = {"TBP", "TBPM"};
static char *ID3_YEARNEW[2] = {"TYE", "TDRC"};
static char *ID3_DISC[2]    = {"TPA", "TPOS"};
static char *ID3_ARTWORK[2] = {"PIC", "APIC"};
static char *ID3_COMPOSER[2]= {"TCM", "TCOM"};
static char *ID3_URL[2]     = {"TID", "TID "};
static char *ID3_POD_URL[2] = {"WFD", "WFD "};
static char *ID3_DESC[2]    = {"TDS", "TDS "};
static char *ID3_RELEASE[2] = {"TDR", "TDR "};

static int find_id3 (int version, FILE *fh, unsigned char *tag_data, int *tag_datalen, int *major_version);
static void one_pass_parse_id3 (FILE *fh, unsigned char *tag_data, int tag_datalen, int version,
                                int id3v2_majorversion, tihm_t *tihm);

static int synchsafe_to_int (unsigned char *buf, int nbytes) {
  int id3v2_len = 0;
  int error = 0;
  int i;

  for (i = 0 ; i < nbytes ; i++) {
    id3v2_len <<= 7;
    id3v2_len += buf[i] & 0x7f;
    if (buf[i] & 0x80)
      error = 1;
  }

  if (error) {
    id3v2_len = 0;
    for (i = 0 ; i < nbytes ; i++) {
      id3v2_len <<= 8;
      id3v2_len += buf[i] & 0xff;
    }
  }

  return id3v2_len;
}

int id3v2_size (unsigned char data[14]) {
  int major_version;
  unsigned char id3v2_flags;
  int id3v2_len = 0;
  int head;

  memcpy (&head, data, 4);
  head = big32_2_arch32 (head);

  if ((head & 0xffffff00) == 0x49443300) {
    major_version = head & 0xff;
    
    id3v2_flags = data[5];
    id3v2_len   = 10 + synchsafe_to_int (&data[6], 4);

    if (id3v2_flags & ID3FLAG_EXTENDED) {
      /* Skip extended header */
      if (major_version != 3)
	id3v2_len += synchsafe_to_int (&data[10], 4);
      else
	id3v2_len += big32_2_arch32 (((int *)&data[10])[0]);
    }
    
    /* total length = 10 (header) + extended header length (flag 0x40) + id3v2len + 10 (footer, if present -- flag 0x10) */
    id3v2_len += (id3v2_flags & ID3FLAG_FOOTER) ? 10 : 0;
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
static int find_id3 (int version, FILE *fh, unsigned char *tag_data, int *tag_datalen, int *major_version) {
  int head;

  fread(&head, 4, 1, fh);    
  head = big32_2_arch32 (head);

  if (version == 2) {
    unsigned char data[10];
    unsigned char id3v2_flags;
    int id3v2_len;
    int id3v2_extendedlen;

    /* version 2 */
    if ((head & 0xffffff00) == 0x49443300) {
      fread(data, 1, 10, fh);

      *major_version = head & 0xff;
      
      id3v2_flags = data[1];
      id3v2_len   = synchsafe_to_int (&data[2], 4);

      /* the 6th bit of the flag field being set indicates that an
	 extended header is present */
      if (id3v2_flags & ID3FLAG_EXTENDED) {
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
	
      mp3_debug ("find_id3: found id3v2.%i tag of size %i\n", *major_version, *tag_datalen);

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
	
      head = big32_2_arch32 (head);
    }
      
    /* version 1 */
    if ((head & 0xffffff00) == 0x54414700) {
      mp3_debug ("find_id3: found id3v1 tag.\n");

      fread(tag_data, 1, 128, fh);
	
      return 1;
    }
  }
    
  /* no id3 found */
  return 0;
}

char *id3v1_string (signed char *unclean, int *length) {
  int i;
  static char buffer[31];

  memset (buffer, 0, 31);

  for (i = 0 ; i < 30 && unclean[i] != -1 ; i++)
    buffer[i] = unclean[i];

  *length = i;

  return buffer;
}

#if defined(HAVE_LIBWAND)
int read_artwork (FILE *fh, tihm_t *tihm, size_t image_size) {
  u_int64_t cksum;

  mp3_debug ("Reading cover art. Image size is %i B\n", image_size);

  tihm->image_data = (unsigned char *)calloc (1, image_size);
  fread (tihm->image_data, 1, image_size, fh);

  cksum = upod_crc64 (tihm->image_data, image_size);

  tihm->has_artwork = 1;
  
  /* a checksum is used for the image id to avoid duplicate images in the database */
  tihm->artwork_id  = cksum;
  tihm->image_size  = image_size;

  return 0;
}
#endif

static int parse_artwork (tihm_t *tihm, FILE *fh, size_t length, int id3v2_majorversion) {
#if defined(HAVE_LIBWAND)
  if (tihm->image_data)
    return 0;

  length --;

  fseek (fh, -length, SEEK_CUR);

  /* Skip past type string */
  for ( ; fgetc (fh) != '\0'; length--);

  /* Seek to the start of the image data */
  length -= (id3v2_majorversion > 2) ? 3 : 2;
  fseek (fh, (id3v2_majorversion > 2) ? 2 : 1, SEEK_CUR);

  return read_artwork (fh, tihm, length);
#else
  mp3_debug ("Cover artwork found and ignored (libupod compiled without libwand).\n");

  return 0;
#endif
}

/*
  parse_id3
*/
static void one_pass_parse_id3 (FILE *fh, unsigned char *tag_data, int tag_datalen, int version,
				int id3v2_majorversion, tihm_t *tihm) {
  int i;
  char *tag_temp;
  int length = 0;

  if (version == 2) {
    char encoding[11];
    char enc_type;
    char identifier[5];
    int newv = (id3v2_majorversion > 2) ? 1 : 0;
    int ident_length = (id3v2_majorversion > 2) ? 4 : 3;

    memset (identifier, 0, 5);

    for (i = 0 ; i < tag_datalen ; ) {
      int ipod_type = -1;
      char *slash;

      fread (tag_data, 1, (id3v2_majorversion > 2) ? 10 : 6, fh);
      
      if (tag_data[0] == 0)
	return;

      memcpy (identifier, tag_data, newv ? 4 : 3);

      /* calculate tag size */
      if (id3v2_majorversion > 2) {
	/* id3v2.3 does not use synchsafe integers in frame headers. */
	if (id3v2_majorversion == 3 || strcmp (identifier, "APIC") == 0 ||
	    strcmp (identifier, "COMM") == 0 || strcmp (identifier, "COM ") == 0 ||
	    strcmp (identifier, "GEOB"))
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
      } else if (length < 2) {
	fseek (fh, length, SEEK_CUR);

	continue;
      }

      /* read the first TAG_DATA_SIZE bytes of the tag (TAG_DATA_SIZE is an arbitrary number) */
      memset (tag_data, 0, TAG_DATA_SIZE);
      fread (tag_data, 1, (length < TAG_DATA_SIZE) ? length : TAG_DATA_SIZE, fh);

      if (length > TAG_DATA_SIZE)
	fseek (fh, length - TAG_DATA_SIZE, SEEK_CUR);

      tag_temp = (char *)tag_data;

      enc_type = *tag_temp;

      if (strcmp (identifier, ID3_ARTWORK[newv]) != 0 && length < TAG_DATA_SIZE) {
	for ( ; length && *tag_temp == '\0' ; tag_temp++, length--);
	/* strip off any trailing \0's */
	for ( ; length && *(tag_temp+length-1) == '\0' ; length--);

	if (length == 0)
	  continue;
      }

      /* detect the tag's encoding */
      switch (enc_type) {
      case 0x00:
	sprintf (encoding, "UTF-8");
	break;
      case 0x01:
	sprintf (encoding, "UTF-16LE");
	/* skip 0x01ff */
	tag_temp += 3;

	if ((length % 2) != 0)
	  length++;

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

      mp3_debug ("Tag: ident: %s (enc: %s), data: %s, length: %i\n", identifier, encoding, tag_temp, length);

      if (strcmp (identifier, ID3_TITLE[newv]) == 0)
	ipod_type = IPOD_TITLE;
      else if (strcmp (identifier, ID3_ARTIST[newv]) == 0)
	ipod_type = IPOD_ARTIST;
      else if (strcmp (identifier, ID3_ALBUM[newv]) == 0)
	ipod_type = IPOD_ALBUM;
      else if (strcmp (identifier, ID3_COMMENT[newv]) == 0)
	ipod_type = IPOD_COMMENT;
      else if (strcmp (identifier, ID3_COMPOSER[newv]) == 0)
	ipod_type = IPOD_COMPOSER;
      else if (strcmp (identifier, ID3_URL[newv]) == 0)
	ipod_type = IPOD_URL;
      else if (strcmp (identifier, ID3_DESC[newv]) == 0)
	ipod_type = IPOD_DESCRIPTION;
      else if (strcmp (identifier, ID3_POD_URL[newv]) == 0) {
	ipod_type = IPOD_PODCAST_URL;
	mp3_debug ("This is probably a podcast.\n");
	tihm->is_podcast = 1;
      } else if (strcmp (identifier, ID3_RELEASE[newv]) == 0) {
	struct tm release_date;

	memset (&release_date, 0, sizeof (struct tm));

	sscanf (tag_temp, "%04d-%02d-%02dT%02d:%02d:%02iZ", &release_date.tm_year,
		&release_date.tm_mon, &release_date.tm_mday, &release_date.tm_hour,
		&release_date.tm_min, &release_date.tm_sec);

	release_date.tm_year -= 1900;
	release_date.tm_mon  -= 1;
	release_date.tm_isdst = -1;
	release_date.tm_zone = "UTC";

	tihm->year = release_date.tm_year;
	tihm->release_date = mktime (&release_date);
      } else if (strcmp (identifier, ID3_ARTWORK[newv]) == 0)
	parse_artwork (tihm, fh, length, id3v2_majorversion);
      else if (strcmp (identifier, ID3_TRACK[newv]) == 0) {
	/* some id3 tags have track/total tracks in the TRCK field */
	tihm->track = strtol (tag_temp, NULL, 10);

	/* set total number of album tracks */
	if ((slash = strchr (tag_temp, '/')) != NULL)
	  tihm->album_tracks = strtol (slash+1, NULL, 10);
      } else if (strcmp (identifier, ID3_DISC[newv]) == 0) {
	/* some id3 tags have disk_num/total_disks in the TPOS field */
	slash = strchr (tag_temp, '/');

	if (slash) *slash = 0;

	tihm->disk_num = strtol (tag_temp, NULL, 10);

	/* set total number of album tracks */
	if ((slash = strchr (tag_temp, '/')) != NULL)
	  tihm->disk_total = strtol (slash+1, NULL, 10);
      } else if (strcmp (identifier, ID3_BPM[newv]) == 0) {
	tihm->bpm = strtol (tag_temp, NULL, 10);
      } else if (strcmp (identifier, ID3_YEARNEW[newv]) == 0 ||
	       strcmp (identifier, ID3_YEAR[newv]) == 0) {
	tihm->year = strtol (tag_temp, NULL, 10);
      } else if (strcmp (identifier, ID3_GENRE[newv]) == 0) {
	if (tag_temp[0] != '(') {
	  dohm_add (tihm, (u_int8_t *)tag_temp, length, encoding, IPOD_GENRE);
	} else {
	  int genre;

	  if ((genre = strtol (tag_temp + 1, NULL, 10)) > 147)
	    continue;

	  ipod_type = IPOD_GENRE;
	  tag_temp = genre_table[genre];
	  length = strlen (genre_table[genre]);
	  sprintf (encoding, "ISO-8859-1");
	}
      }

      if (ipod_type != -1)
	dohm_add (tihm, (u_int8_t *)tag_temp, length, encoding, ipod_type);
    }
  } else if (version == 1) {
    tag_temp = id3v1_string ((signed char *)&tag_data[3], &length);
    dohm_add (tihm, (u_int8_t *)tag_temp, length, "ISO-8859-1", IPOD_TITLE);

    tag_temp = id3v1_string ((signed char *)&tag_data[33], &length);
    dohm_add (tihm, (u_int8_t *)tag_temp, length, "ISO-8859-1", IPOD_ARTIST);

    tag_temp = id3v1_string ((signed char *)&tag_data[33], &length);
    dohm_add (tihm, (u_int8_t *)tag_temp, length, "ISO-8859-1", IPOD_ALBUM);

    tag_temp = id3v1_string ((signed char *)&tag_data[93], &length);
    dohm_add (tihm, (u_int8_t *)tag_temp, length, "ISO-8859-1", IPOD_COMMENT);
    
    if ((tag_data[126] != 0xff) && (tihm->track == 0))
      tihm->track = tag_data[126];
    
    if ((signed char)tag_data[127] != -1)
      dohm_add (tihm, (u_int8_t *)genre_table[tag_data[127]], strlen(genre_table[tag_data[127]]), "ISO-8859-1", IPOD_GENRE);
  }
}

int get_id3_info (FILE *fh, char *file_name, tihm_t *tihm) {
  int tag_datalen = 0;
  unsigned char tag_data[TAG_DATA_SIZE];
  int version;
  int id3v2_majorversion;

  /* built-in id3tag reading -- id3v2, id3v1 */
  if ((version = find_id3(2, fh, tag_data, &tag_datalen, &id3v2_majorversion)) != 0)
    one_pass_parse_id3(fh, tag_data, tag_datalen, version, id3v2_majorversion, tihm);

  /* some mp3's have both tags so check v1 even if v2 is available */
  if ((version = find_id3(1, fh, tag_data, &tag_datalen, &id3v2_majorversion)) != 0)
    one_pass_parse_id3(fh, tag_data, tag_datalen, version, id3v2_majorversion, tihm);
  
  return 0;
}

