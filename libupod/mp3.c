/**
 *   (c) 2001 Nathan Hjelm <hjelmn@unm.edu>
 *   v1.0.6 mp3.c 
 *
 *   Changes:
 *     11-28-2001
 *       - Function no longer cuts off ID3 tags at the beginning.
 *       - Now doesn't need id3lib to parse tags
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

#include <string.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

/* from mpg123 */
#include "genre.h"

#ifdef linux
#include <byteswap.h>
#include <endian.h>
#elif defined(__FreeBSD__) || defined(__MacOSX__)
#include <machine/endian.h>
#endif

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "upodi.h"

/*
 * Decoding MP3 frame headers is discussed here:
 * http://www.mp3-tech.org/programmer/frame_header.html
 *
 * Also see common.c from mpg123
 */

/**
 * Index elements are [MPEG VERSION][LAYER][BITRATEINDEX]
 * MPEG version 1.0 == 0, MPEG version 2.0 and 2.5 == 1
 */
static const int MP3_BITRATES[2][3][16] = {
    /* MPEG Version 1.0 */
    {   
	/* Layer I */
	{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
	/* Layer II */
	{0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
	/* Layer III */
	{0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,}
    },    
    /* MPEG Version 2.0 and 2.5 */
    {   
	/* Layer I */
	{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
	/* Layer II */
	{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
	/* Layer III */
	{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,}
    }
};

/**
 * Sampling rate frequency index 
 * 
 * Index elements are [MPEG_VERSION][FREQUENCY_INDEX]
 * MPEG version 1.0 == 3, MPEG version 2.0 == 1, MPEG version 2.5 == 0
 * index 01 == reserved
 */
static const long MP3_FREQS[4][4] = { 
    /* MPEG Version 2.5 */
    {
	11025 , 12000 , 8000, 0  /* 0 == reserved */
    },
    /* Reserved */
    {
	0, 0, 0, 0
    },
    /* MEPG Version 2.0 */
    {
	22050, 24000, 16000, 0  /* 0 == reserved */
    },
    /* MPEG Version 1.0 */
    {
	44100, 48000, 32000, 0  /* 0 == reserved */
    }
};

static const char *MPEG_LAYERS[4] = { "(reserved)", "III", "II", "I"};

#define MPEG_LAYER_I   3
#define MPEG_LAYER_II  2
#define MPEG_LAYER_III 1

static const char *MPEG_VERSION_NAMES[4] = {
  "2.5", "(reserved)", "2.0", "1.0"
};

#define MPEG_VERSION_1        3
#define MPEG_VERSION_2        2
#define MPEG_VERSION_RESERVED 1
#define MPEG_VERSION_25       0

#define ID3_TITLE             0
#define ID3_TALT              1
#define ID3_ARTIST            2
#define ID3_ALBUM             3
#define ID3_TRACK             4
#define ID3_YEAR              5
#define ID3_GENRE             6
#define ID3_ENCODER           7
#define ID3_COMMENT           8

static int  find_id3(int fd, char **tag_data, int *tag_datalen);
static int  parse_id3(char *tag_data, int tag_datalen, int version, int field, char *dst);

/**
 * Returns the MP3 layer from the provided header.
 */
static int getMP3Layer(unsigned long head) {
  /* Bits (18,17)
     Layer description
     00 - reserved
     01 - Layer III
     10 - Layer II
     11 - Layer I 
  */
  unsigned long mask = (1 << 17) | (1 << 18);
  int layer = (head & mask) >> 17;

  if ( layer == 0 )
    layer = -1;

  return layer;
}

/**
 * Returns the MPEG version from the provided header.
 */
static int getMPEGVersion(unsigned long head) {
  /* Bits (20, 19)
     MPEG Audio version ID
     00 - MPEG Version 2.5 (later extension of MPEG 2)
     01 - reserved
     10 - MPEG Version 2 (ISO/IEC 13818-3)
     11 - MPEG Version 1 (ISO/IEC 11172-3) 
                     
     Note: MPEG Version 2.5 was added lately to the MPEG 2 standard. 
     		It is an extension used for very low bitrate files, allowing 
    		the use of lower sampling frequencies. If your decoder does 
    		not support this extension, it is recommended for you to use 
    		12 bits for synchronization instead of 11 bits. 
  */
  unsigned long mask = (1 << 20) | (1 << 19);
  int version = (head & mask) >> 19;

  if ( version == 1 )
    version = -1;

  return version;
}

/**
 * Returns the MP3 bitrate index from the provided header.
 */
static int getMP3BitrateIndex(unsigned long head) {
  /* Bits (15 to 12)
     see table above for full values */
  unsigned long mask = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
  int index = (head & mask) >> 12;
  if (index == 0xF)
    index = -1;

  return index;
}

/** 
 * Returns the sampling frequency from the provided header.
 */
static int getMP3SampFrequency(unsigned long head) {
  /* Bits (11, 10)
     see table above for decode of the values.
     1,1 == reserved */
  unsigned long mask = (1 << 11) | (1 << 10);
  int freq = (head & mask) >> 10;
  if (freq == 0x3)
    freq = -1;

  return freq;
}

/* head_check is from mpg123 0.59r common.c */
static int head_check(unsigned long head) {
  /* Bits (31 to 21) must all be set/ */ 
  if ( (head & 0xffe00000) != 0xffe00000 )
    return 0;
  /* makes sure the mp3 layer is not "00 - reserved" 
     (right now the Rio only supports Layer III MPEG's) */
  if ( ((head>>17)&3) != MPEG_LAYER_III )     
    return 0;                  
  /* check to make sure that the bit rate is correct. */
  /* "1111 - reserved is not accepted" */
  if ( ((head>>12)&0xf) == 0xf ) 
    return 0;                
  /* Make sure the sampling freq. index is not "11 - reserved" */
  if ( ((head>>10)&0x3) == 0x3 ) 
    return 0;
  if ( (head & 0xffff0000) == 0xfffe0000 )
    return 0;

  return 1;
}

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

  return vals:
  -1: not found
   0: found
*/
static int parse_id3 (char *tag_data, int tag_datalen, int version,
		      int field, char *dst) {
  char *tmpc;
  
  if (version == 2) {
    char *fields[]     = {"TT1", "TT2", "TP1", "TAL", "TRK", "TYE", "TCO",
			  "TEN", "COM", "TLE", "TKE"};
    char *fourfields[] = {"TIT1", "TIT2", "TPE1", "TALB", "TRCK", "TYER",
			  "TCON", "TENC", "COMM", "TLEN", "TIME"};
	
    char *tag_temp;
    char *sizeloc;
    char genre_temp[4];
    
    /* look for a needle in a haystack */
    if ( ((tag_temp = (char *)memmem(tag_data, tag_datalen,
				     fields[field], 0x03)) != NULL ) ||
	 ((tag_temp = (char *)memmem(tag_data, tag_datalen,
				     fourfields[field], 0x03)) != NULL) ) {
      
      for (tag_temp += 4 ; (*tag_temp == 0) ; tag_temp++);
      sizeloc = tag_temp;
      /* skip the flags field */
      for (tag_temp += 2 ; (*tag_temp == 0) ; tag_temp++);
      
      if ((field != ID3_GENRE) || ( *(tag_temp) != 40) ) {
	int length = 0;
	
	length = *(sizeloc) - 1;
	
	memset(dst, 0, 63);
	
	if (dst)
	  strncpy(dst, tag_temp, (length > 63) ? 63 : length);
	
      } else {
	int i;
	
	/* character 41 is right parenthesis */
	for (i = 0 ; (*(tag_temp + 1 + i) != 41) ; i++) {
	  genre_temp[i] = *(tag_temp + 1 + i);
	}
	
	genre_temp[i] = 0;
	
	if (dst)
	  strncpy(dst, genre_table[atoi(genre_temp)], 30);
      }   
    } else {
      dst[0] = 0;
      return -1;
    }
  } else if (version == 1) {
    int i = 29;

    switch (field) {
    case ID3_TITLE:
      strncpy(dst, &tag_data[3], 30);
      break;
    case ID3_ARTIST:
      strncpy(dst, &tag_data[33], 30);
      break;
    case ID3_ALBUM:
      strncpy(dst, &tag_data[63], 30);
      break;
    case ID3_COMMENT:
      strncpy(dst, &tag_data[97], 30);
      break;
    case ID3_GENRE:
      if (tag_data[127] > genre_count)
	break;

      strncpy(dst, genre_table[tag_data[127]],
	      strlen(genre_table[tag_data[127]]));
      break;
    default:
      return -1;
    }
    
    while((i >= 0) && (dst[i] == ' '))
      dst[i--] = 0;	

    if (strlen(dst) == 0)
      return -1;
  }
  
  return 0;
}

/*
  mp3_info
*/
int ref_fill_mp3 (char *file_name, song_ref_t *new_entry){
  unsigned char buf[128];
  int folder_number;
  int i, j, ret;
  int tag_datalen = 0;

  struct stat statinfo;

  int freq_index, bitrate_index, layer, version;

  int fd;
  u_int32_t head, nexthead;

  if (new_entry == NULL)
    return -1;

  /* get file size */
  if (stat(file_name, &statinfo) < 0)
    return -1;

  new_entry->size = statinfo.st_size;


  if ((fd = open (file_name, O_RDONLY)) < 0)
    return errno;

  {
    char *tag_data = NULL;
    char artist[255], title[255], album[255], genre[255];
    char comment[255], path[255];
    char audio_type[] = "MPEG audio file";
    int  version;
    int  current_mhod = 0;

    artist[0] = title[0] = album[0] = genre[0] = comment[0] = '\0';
    
    /* there are three mhod that we will always have: path, type, title */
    new_entry->dohm_num = 3;
    
    if ((version = find_id3(fd, &tag_data, &tag_datalen)) != 0) {
      parse_id3(tag_data, tag_datalen, version, ID3_TITLE, title);
      
      if ( (version == 2) && (strlen(title) == 0))
	parse_id3(tag_data, tag_datalen, version, ID3_TALT, title);
      
      if (parse_id3(tag_data, tag_datalen, version, ID3_ARTIST, artist) == 0)
	new_entry->dohm_num++;
      if (parse_id3(tag_data, tag_datalen, version, ID3_ALBUM, album) == 0)
	new_entry->dohm_num++;
      if (parse_id3(tag_data, tag_datalen, version, ID3_COMMENT, comment) == 0)
	new_entry->dohm_num++;
      if (parse_id3(tag_data, tag_datalen, version, ID3_GENRE, genre) == 0)
	new_entry->dohm_num++;
      
      free(tag_data);
    }

    new_entry->dohm_entries = (struct dohm *)
      malloc(new_entry->dohm_num * sizeof(struct dohm));

    new_entry->dohm_entries[current_mhod].type = TYPE;
    new_entry->dohm_entries[current_mhod].data = malloc(strlen(audio_type));
    memcpy(new_entry->dohm_entries[current_mhod++].data, 
   	   audio_type, strlen(audio_type));

    {
      srand(time(NULL));
      folder_number = rand()%20;

      sprintf(path, "/iPod_Control/Music/F%02i/%s", folder_number,
	      basename(file_name));
      new_entry->dohm_entries[current_mhod].type = PATH;
      new_entry->dohm_entries[current_mhod].data = malloc(strlen(path));
      memcpy(new_entry->dohm_entries[current_mhod++].data, path, strlen(path));
    }

    new_entry->dohm_entries[current_mhod].type = TITLE;

    if (strlen(title) == 0)
      sprintf(title, "%s", basename(file_name));
    
    new_entry->dohm_entries[current_mhod].data = malloc(strlen(title));
    memcpy(new_entry->dohm_entries[current_mhod++].data,
	   title, strlen(title));

    if (strlen(artist) > 0) {
      new_entry->dohm_entries[current_mhod].type = ARTIST;
      new_entry->dohm_entries[current_mhod].data = malloc(strlen(artist));
      memcpy(new_entry->dohm_entries[current_mhod++].data,
	     artist, strlen(artist));
    }

    if (strlen(album) > 0) {
      new_entry->dohm_entries[current_mhod].type = ALBUM;
      new_entry->dohm_entries[current_mhod].data = malloc(strlen(album));
      memcpy(new_entry->dohm_entries[current_mhod++].data,
	     album, strlen(album));
    }

    if (strlen(comment) > 0) {
      new_entry->dohm_entries[current_mhod].type = COMMENT;
      new_entry->dohm_entries[current_mhod].data = malloc(strlen(comment));
      memcpy(new_entry->dohm_entries[current_mhod++].data,
	     comment, strlen(comment));
    }

    if (strlen(genre) > 0) {
      new_entry->dohm_entries[current_mhod].type = GENRE;
      new_entry->dohm_entries[current_mhod].data = malloc(strlen(genre));
      memcpy(new_entry->dohm_entries[current_mhod++].data,
	     genre, strlen(genre));
    }
  }

  /* got this from mpg123 as well */
  for (i = 0 ; i < (65536 + tag_datalen) ; i++){
    read(fd, &head, 4);
#if BYTE_ORDER == LITTLE_ENDIAN
    head = bswap_32(head);
#endif

    if (head_check(head)) break;

    lseek(fd, -3, SEEK_CUR);  
  }

  j = i;

  /* double check that we have the right head */
  for (; i < (65536 + tag_datalen) ; i++){
    read(fd, &nexthead, 4);
#if BYTE_ORDER == LITTLE_ENDIAN
    head = bswap_32(nexthead);
#endif

    if (head_check(nexthead)) break;
    
    lseek(fd, -3, SEEK_CUR);  
  }

  /* finished with the file */
  close(fd);

  if ( (nexthead & 0xffff0000) == (head & 0xffff0000) )
    i = j;
  else
    head = nexthead;
  /* end of double check */

  if (i == (65536 + tag_datalen)) {
    ref_clear(new_entry);
    return -2;
  }

  freq_index    = getMP3SampFrequency(head);
  layer         = getMP3Layer(head);
  version       = getMPEGVersion(head);
  bitrate_index = getMP3BitrateIndex(head);

  /* Make sure that the values we extracted are correct. All routines return -1
     on error and print an error message so all we have to do is bail out. */
  if (freq_index == -1 || layer == -1 || 
      version == -1 || bitrate_index == -1) {
    ref_clear(new_entry);
    return -2;
  }

  if ( version == MPEG_VERSION_1 )
    new_entry->bitrate = MP3_BITRATES[0][3 - layer][bitrate_index];
  else
    new_entry->bitrate = MP3_BITRATES[1][3 - layer][bitrate_index];
  


  new_entry->samplerate =  MP3_FREQS[version][freq_index];
  new_entry->time = (u_int32_t)((double)(new_entry->size - i)/
			      ((double)(new_entry->bitrate) * 125));


  new_entry->mod_date = time(NULL);

  return 0;
}
