/**
 *   (c) 2002 Nathan Hjelm <hjelmn@unm.edu>
 *   v0.1.0a mp3.c 
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

#include "upodi.h"

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

/* from mpg123 */
#include "genre.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "mp3tech.h"

#define ID3_TITLE             0
#define ID3_TALT              1
#define ID3_ARTIST            2
#define ID3_ALBUM             3
#define ID3_TRACK             4
#define ID3_YEAR              5
#define ID3_GENRE             6
#define ID3_ENCODER           7
#define ID3_COMMENT           8

static int find_id3 (int fd, char **tag_data, int *tag_datalen) {
    int head;
    char data[10];
    int id3v2len;

    read(fd, &head, 4);
    lseek(fd, 0, SEEK_CUR);

#if BYTE_ORDER == LITTLE_ENDIAN
    head = bswap_32(head);
#endif


    /* version 2 */
    if ( (head & 0x49443300) == 0x49443300) {
	read(fd, data, 10);
	
	id3v2len  = (unsigned long) data[2] & 0x7f;
	id3v2len <<= 7;
	id3v2len += (unsigned long) data[3] & 0x7f;
	id3v2len <<= 7;
	id3v2len += (unsigned long) data[4] & 0x7f;
	id3v2len <<= 7;
	id3v2len += (unsigned long) data[5] & 0x7f;
	
	id3v2len += 10;

	*tag_datalen = id3v2len;

	*tag_data = malloc(id3v2len);

	lseek(fd, 0, SEEK_SET);
	read(fd, *tag_data, id3v2len);
	lseek(fd, 0, SEEK_SET);

	return 2;
    }

    lseek(fd, 0, SEEK_SET);

    /* tag not at beginning? */
    if ( (head & 0x54414700) != 0x54414700) {
	/* maybe end */
	lseek(fd, -128, SEEK_END);
	read(fd, &head, 4);
	lseek(fd, -128, SEEK_END);

#if BYTE_ORDER == LITTLE_ENDIAN
	head = bswap_32(head);
#endif
    }

    /* version 1 */
    if ( (head & 0x54414700) == 0x54414700) {
	*tag_datalen = 128;
	*tag_data = malloc(128);

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
static void parse_id3 (char *tag_data, int tag_datalen, int version, int field, tihm_t *tihm) {
  char *tmpc;
  dohm_t *dohm;

  dohm = dohm_create(tihm);
  
  switch (field) {
  case ID3_ARTIST:
    dohm->type = IPOD_ARTIST;
    break;
  case ID3_TALT:
  case ID3_TITLE:
    dohm->type = IPOD_TITLE;
    break;
  case ID3_GENRE:
    dohm->type = IPOD_GENRE;
    break;
  case ID3_ALBUM:
    dohm->type = IPOD_ALBUM;
    break;
  case ID3_COMMENT:
    dohm->type = IPOD_COMMENT;
    break;
  default:
    return;
  }

  if (version == 2) {
    char *fields[]     = {"TT1", "TT2", "TP1", "TAL", "TRK", "TYE", "TCO",
			  "TEN", "COM", "TLE", "TKE"};
    char *fourfields[] = {"TIT1", "TIT2", "TPE1", "TALB", "TRCK", "TYER", "TCON",
			  "TENC", "COMM", "TLEN", "TIME"};
    
    char *tag_temp;
    char *sizeloc;
    char genre_temp[4];
    
    /* look for a needle in a haystack */
    if ( ((tag_temp = (char *)memmem(tag_data, tag_datalen, fields[field], 0x03)) != NULL ) ||
	 ((tag_temp = (char *)memmem(tag_data, tag_datalen, fourfields[field], 0x03)) != NULL) ) {
      
      for (tag_temp += 4 ; (*tag_temp == 0) ; tag_temp++);
      sizeloc = tag_temp;
      /* skip the flags field */
      for (tag_temp += 2 ; (*tag_temp == 0) ; tag_temp++);
      
      if ((field != ID3_GENRE) || ( *(tag_temp) != 40) ) {
	int length = 0;
	
	length = *(sizeloc) - 1;
	
	dohm->size = 2 * length;
	dohm->data = calloc (1, dohm->size);
	
	if (dohm->data != NULL)
	  char_to_unicode (dohm->data, tag_temp, length);
	else
	  goto id3_error;
	
      } else {
	int i;
	
	/* 41 is right parenthesis */
	for (i = 0 ; (*(tag_temp + 1 + i) != 41) ; i++) {
	  genre_temp[i] = *(tag_temp + 1 + i);
	}
	
	genre_temp[i] = 0;
	
	dohm->size = 2 * strlen(genre_table[atoi(genre_temp)]);
	dohm->data = calloc (1, dohm->size);
	
	if (dohm->data != NULL)
	  char_to_unicode (dohm->data, genre_table[atoi(genre_temp)], dohm->size/2);
	else
	  goto id3_error;
      }   
    } else {
      goto id3_not_found;
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
    case ID3_GENRE:
      copy_from = genre_table[tag_data[127]];
      i = strlen (copy_from - 1);
    default:
      goto id3_not_found;
    }
    
    for (tmp = copy_from + i ; *tmp == ' ' && i >= 0; tmp--, i--)
      *tmp = 0;
	
    if (i < 0)
      goto id3_not_found;
    
    i++;

    dohm->size = 2 * i;
    dohm->data = calloc(1, dohm->size);
    char_to_unicode (dohm->data, copy_from, i);
  }

    return;
 id3_not_found:
 id3_error:
    dohm_destroy(tihm);
}

static int get_id3_info (char *file_name, tihm_t *tihm) {
  int fd;
  int tag_datalen = 0;
  char *tag_data = NULL;
  int version;
  
  if ((fd = open (file_name, O_RDONLY)) < 0)
    return errno;
  
  /* ** NEW ** built-in id3tag reading -- id3v2, id3v1 */
  if ((version = find_id3(fd, &tag_data, &tag_datalen)) != 0) {
    parse_id3(tag_data, tag_datalen, version, ID3_TITLE, tihm);
    
    /* Much of the time the title is in field TT2 not TT1 */
    if ( (version == 2) && (tihm->num_dohm == 0))
      parse_id3(tag_data, tag_datalen, version, ID3_TALT, tihm);
    
    
    parse_id3(tag_data, tag_datalen, version, ID3_ARTIST , tihm);
    parse_id3(tag_data, tag_datalen, version, ID3_ALBUM  , tihm);
    parse_id3(tag_data, tag_datalen, version, ID3_COMMENT, tihm);
    parse_id3(tag_data, tag_datalen, version, ID3_GENRE  , tihm);

    free(tag_data);
  }
  
  if (tihm->num_dohm == 0 || tihm->dohms[0].type != IPOD_TITLE) {
    char *tmp = basename(file_name);
    dohm_t *dohm;
    int i;
    
    for (i=strlen(tmp)-1; (tmp[i] != '.') && i > 0 ; i--);
    
    dohm = dohm_create(tihm);

    dohm->type = IPOD_TITLE;
    dohm->size = 2 * (strlen(tmp) - (i - 1));
    dohm->data = calloc(1, dohm->size);
    char_to_unicode (dohm->data, tmp, dohm->size/2);
  }
  
  close(fd);

  return version;
}

/*
  Fills in the mp3_file structure and returns the file offset to the 
  first MP3 frame header.  Returns >0 on success; -1 on error.
   
  This routine is based in part on MP3Info 0.8.4.
  MP3Info 0.8.4 was created by Cedric Tefft <cedric@earthling.net> 
  and Ricardo Cerqueira <rmc@rccn.net>
*/
static int get_mp3_header_info (char *file_name, tihm_t *tihm) {
  int scantype=SCAN_QUICK, fullscan_vbr=1;
  mp3info mp3;

  memset(&mp3,0,sizeof(mp3info));
  mp3.filename=file_name;

  if ( !( mp3.file=fopen(file_name,"r") ) ) {
    fprintf(stderr,"Error opening MP3 file: %s\n",file_name);
    return -1;
  } 

  get_mp3_info(&mp3, scantype, fullscan_vbr);
  if(!mp3.header_isvalid) {
    fclose(mp3.file);
    fprintf(stderr,"%s is corrupt or is not a standard MP3 file.\n",
	    mp3.filename);
    return -1;
  }
  
  /* the ipod wants time in thousands of seconds */
  tihm->time        = mp3.seconds * 1000;
  tihm->samplerate  = header_frequency(&mp3.header);  
  tihm->bitrate     = header_bitrate(&mp3.header); 
  
  fclose(mp3.file);
  return 0;
}

/*
  mp3_fill_tihm: fills a tihm structure for adding a file to the iTunesDB.
*/
int mp3_fill_tihm (char *file_name, tihm_t *tihm){
  struct stat statinfo;

  int id3_version;
  int mp3_header_offset;

  char type_string[] = "MPEG audio file";
  
  int ret;

  dohm_t *dohm;

  memset (tihm, 0, sizeof(tihm_t));

  ret = stat(file_name, &statinfo);

  if (ret < 0) {
    perror("mp3_fill_tihm");
    return -1;
  }

  tihm->size = statinfo.st_size;

  if ((id3_version = get_id3_info(file_name, tihm)) < 0)
    return -1;
  
  if ((mp3_header_offset = get_mp3_header_info(file_name, tihm)) < 0)
    return -1;

  dohm = dohm_create(tihm);

  dohm->type = IPOD_TYPE;
  dohm->size = 2 * strlen (type_string);
  dohm->data = calloc(1, dohm->size);
  char_to_unicode (dohm->data, type_string, strlen(type_string));

  return 0;
}
