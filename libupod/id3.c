/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a id3.c 
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

static int find_id3 (int fd, char **tag_data, int *tag_datalen, int *major_version);
static void parse_id3 (char *tag_data, int tag_datalen, int version, int id3v2_majorversion, int field, tihm_t *tihm);
static void cleanup_id3 (char *tag_data, int tag_datalen, int version);

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
static int find_id3 (int fd, char **tag_data, int *tag_datalen, int *major_version) {
    int head;
    char data[10];

    char id3v2_flags;
    int  id3v2_len;
    int  id3v2_extendedlen;

    read(fd, &head, 4);
    
#if BYTE_ORDER == LITTLE_ENDIAN
    head = bswap_32(head);
#endif

    /* version 2 */
    if ((head & 0xffffff00) == 0x49443300) {
	read(fd, data, 10);

	*major_version = head & 0xff;

	id3v2_flags = data[1];

	id3v2_len = synchsafe_to_int (&data[2], 4);

	/* the 6th bit of the flag field being set indicates that an
	   extended header is present */
	if (id3v2_flags & 0x40) {
	  /* Skip extended header */
	  id3v2_extendedlen = synchsafe_to_int (&data[6], 4);

	  lseek(fd, 0xa + id3v2_extendedlen, SEEK_SET);
	  *tag_datalen = id3v2_len - id3v2_extendedlen;
	} else {
	  /* Skip standard header */
	  lseek(fd, 0xa, SEEK_SET);
	  *tag_datalen = id3v2_len;
	}

	*tag_data = calloc(*tag_datalen, sizeof(char));

	read(fd, *tag_data, *tag_datalen);
	lseek(fd, 0, SEEK_SET);

	return 2;
    }

    lseek(fd, 0, SEEK_SET);

    /* tag not at beginning? */
    if ((head & 0xffffff00) != 0x54414700) {
	/* maybe end */
	lseek(fd, -128, SEEK_END);
	read(fd, &head, 4);
	lseek(fd, -128, SEEK_END);

#if BYTE_ORDER == LITTLE_ENDIAN
	head = bswap_32(head);
#endif
    }

    /* version 1 */
    if ((head & 0xffffff00) == 0x54414700) {
	*tag_datalen = 128;
	*tag_data = calloc(128, sizeof(char));

	read(fd, *tag_data, 128);
	lseek(fd, 0, SEEK_SET);

	return 1;
    }

    /* no id3 found */
    lseek(fd, 0, SEEK_SET);

    return 0;
}

/*
  parse_id3
*/
static void parse_id3 (char *tag_data, int tag_datalen, int version, int id3v2_majorversion, int field,
		       tihm_t *tihm) {
  int data_type;
  int i;

  switch (field) {
  case ID3_ARTIST:
    data_type = IPOD_ARTIST;
    break;
  case ID3_TALT:
  case ID3_TITLE:
    data_type = IPOD_TITLE;
    break;
  case ID3_GENRE:
    data_type = IPOD_GENRE;
    break;
  case ID3_ALBUM:
    data_type = IPOD_ALBUM;
    break;
  case ID3_COMMENT:
    data_type = IPOD_COMMENT;
    break;
  case ID3_TRACK:
    break;
  default:
    return;
  }

  if (version == 2) {
    /* field tags associated with id3v2 with major version <= 2 */
    char *fields[]     = {"TT1", "TT2", "TP1", "TAL", "TRK", "TYE", "TCO",
			  "TEN", "COM", "TLE", "TKE"};
    /* field tags associated with id3v2 with major version > 2 */
    char *fourfields[] = {"TIT1", "TIT2", "TPE1", "TALB", "TRCK", "TYER", "TCON",
			  "TENC", "COMM", "TLEN", "TIME"};
    
    char *tag_temp;
    char *sizeloc;
    char genre_temp[4];

    for (i = 0 ; i < tag_datalen ; ) {
      int length = 0;

      tag_temp = NULL;

      if (id3v2_majorversion > 2) {
	length = synchsafe_to_int (&tag_data[i + 4], 4);

	if (strncmp(&tag_data[i], fourfields[field], 4) == 0)
	  tag_temp = &tag_data[i + 10];

	i += 10 + length;
      } else {
	length = synchsafe_to_int (&tag_data[i + 3], 3);

	if (strncmp(&tag_data[i], fields[field], 3) == 0)
	  tag_temp = &tag_data[i + 6 + 1];

	i += 6 + length;
      }

      if (tag_temp == NULL || length < 1)
	continue;

      if ((field != ID3_TRACK) &&
	  ((field != ID3_GENRE) || ( *(tag_temp) != 40)) )
	dohm_add (tihm, tag_temp, length, data_type);
      else if (field == ID3_TRACK) {
	char *slash;

	/* some id3 tags have track/total tracks in the TRK field */
	slash = strchr (tag_temp, '/');

	if (slash) *slash = 0;

	tihm->track = atol (tag_temp);

	if (slash) *slash = '/';
      } else {
	int i;
	
	/* 41 is right parenthesis */
	for (i = 0 ; (*(tag_temp + 1 + i) != 41) ; i++) {
	  genre_temp[i] = *(tag_temp + 1 + i);
	}
	
	genre_temp[i] = 0;

	if (atoi (genre_temp) > 147)
	  return;

	dohm_add (tihm, genre_table[atoi(genre_temp)], strlen (genre_table[atoi(genre_temp)]), data_type);
      }   
    }
  } else if (version == 1) {
    int i = 29;
    char *copy_from, *tmp;

    switch (field) {
    case ID3_TITLE:
      copy_from = &tag_data[3];
      break;
    case ID3_ARTIST:
      copy_from = &tag_data[33];
      break;
    case ID3_ALBUM:
      copy_from = &tag_data[63];
      break;
    case ID3_COMMENT:
      copy_from = &tag_data[93];
      break;
    case ID3_GENRE:
      if ((int)tag_data[127] >= genre_count || (signed char)tag_data[127] == -1)
	return;

      copy_from = genre_table[tag_data[127]];
      i = strlen (copy_from - 1);
      break;
    default:
      return;
    }

    if ((signed char) copy_from[0] == -1)
      return;

    if (field != ID3_GENRE)
      for (tmp = copy_from + i ; (*tmp == ' ' || (signed char)(*tmp) == -1) && i >= 0; tmp--, i--)
	*tmp = 0;
    else
      i = strlen(copy_from) - 1;

    if (i < 0)
      return;
    
    i++;
    dohm_add (tihm, copy_from, i, data_type);
  }
}

static void cleanup_id3 (char *tag_data, int tag_datalen, int version) {
  if (version && tag_data)
    free (tag_data);
}

int get_id3_info (char *file_name, tihm_t *tihm) {
  int fd;
  int tag_datalen = 0;
  char *tag_data = NULL;
  int version;
  int id3v2_majorversion;
  
  if ((fd = open (file_name, O_RDONLY)) < 0)
    return -errno;
  
  /* built-in id3tag reading -- id3v2, id3v1 */
  if ((version = find_id3(fd, &tag_data, &tag_datalen, &id3v2_majorversion)) != 0) {
    parse_id3(tag_data, tag_datalen, version, id3v2_majorversion, ID3_TITLE, tihm);
    
    /* Much of the time the title is in field TT2 not TT1 */
    if ( (version == 2) && (tihm->num_dohm == 0))
      parse_id3(tag_data, tag_datalen, version, id3v2_majorversion, ID3_TALT, tihm);
    
    
    parse_id3(tag_data, tag_datalen, version, id3v2_majorversion, ID3_ARTIST , tihm);
    parse_id3(tag_data, tag_datalen, version, id3v2_majorversion, ID3_ALBUM  , tihm);
    parse_id3(tag_data, tag_datalen, version, id3v2_majorversion, ID3_COMMENT, tihm);
    parse_id3(tag_data, tag_datalen, version, id3v2_majorversion, ID3_GENRE  , tihm);
    parse_id3(tag_data, tag_datalen, version, id3v2_majorversion, ID3_TRACK  , tihm);
  }
  
  if (tihm->num_dohm == 0 || tihm->dohms[0].type != IPOD_TITLE) {
    char *tmp = (char *)basename(file_name);
    int i;
    
    for (i=strlen(tmp)-1; i > 0 ; i--)
      if (tmp[i] == '.') {
	tmp[i] = 0;
	break;
      }
    
    dohm_add (tihm, tmp, strlen(tmp), IPOD_TITLE);
  }
  
  cleanup_id3 (tag_data, tag_datalen, version);

  close(fd);
  
  return 0;
}

