/**
 *   (c) 2002-2004 Nathan Hjelm <hjelmn@cs.unm.edu>
 *   v0.2 mp3.c 
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

/* from mpg123 */
#include "genre.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

struct mp3_file {
  FILE *fh;

  int file_size;  /* Bytes */
  int tagv2_size; /* Bytes */
  int skippage;   /* Bytes */
  int data_size;  /* Bytes */

  int vbr;
  int bitrate;    /* bps */

  int samplerate; /* Hz */

  int length;     /* ms */
};

size_t bitrate_table[] = {
  -1, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320
};

size_t samplerate_table[] = {
  44100, 48000, 32000
};

static int synchsafe_to_int (char *buffer, int len) {
  int i;
  int sum = 0;

  for (i = 0 ; i < len ; i++)
    sum = (sum << 7) | buffer[i];
  
  return sum;
}

static int find_first_frame (struct mp3_file *mp3) {
  int header;
  int buffer;
  mp3->skippage = 0;

  while (fread (&header, 4, 1, mp3->fh)) {
    /* MPEG-1 Layer III */
    if ((header & 0xffea0000) == 0xffea0000) {
      /* Check for Xing frame and skip it */
      fseek (mp3->fh, 32, SEEK_CUR);
      fread (&buffer, 4, 1, mp3->fh);
      if (buffer == ('X' << 24 | 'i' << 16 | 'n' << 8 | 'g')) {
	int bitrate = bitrate_table [((header & 0x0000f000) >> 12)];
	int samplerate = samplerate_table [(header & 0x00000c00) >> 10];
	int frame_size = (size_t) (144000.0 * (double)bitrate/(double)samplerate) + ((header & 0x00000200) >> 9);
	fseek (mp3->fh, frame_size, SEEK_CUR);

	/* an mp3 with an Xing header is ALWAYS vbr */
	mp3->vbr = 1;
      }

      fseek (mp3->fh, -36, SEEK_CUR);
      fseek (mp3->fh, -4, SEEK_CUR);
      return 0;
    }
    
    fseek (mp3->fh, -3, SEEK_CUR);
    mp3->skippage++;
  }

  return -1;
}

static int mp3_open (char *file_name, struct mp3_file *mp3) {
  struct stat statinfo;

  char buffer[5];

  memset (mp3, 0 , sizeof (struct mp3_file));

  if (stat (file_name, &statinfo) < 0)
    return -errno;

  mp3->file_size = mp3->data_size = statinfo.st_size;

  mp3->fh = fopen (file_name, "r");
  if (mp3->fh == NULL) 
    return -errno;

  /* Adjust total_size if an id3v1 tag exists */
  fseek (mp3->fh, -128, SEEK_END);
  memset (buffer, 0, 5);

  fread (buffer, 1, 3, mp3->fh);
  if (strncmp (buffer, "TAG", 3) == 0)
    mp3->data_size -= 128;
  /*                                          */

  fseek (mp3->fh, 0, SEEK_SET);

  /* find and skip id3v2 tag if it exists */
  memset (buffer, 0, 5);
  fread (buffer, 1, 4, mp3->fh);
  if (strncmp (buffer, "ID3", 3) == 0) {
    fseek (mp3->fh, 6, SEEK_SET);
    fread (buffer, 1, 4, mp3->fh);
    
    mp3->tagv2_size = synchsafe_to_int (buffer, 4);

    fseek (mp3->fh, mp3->tagv2_size + 10, SEEK_SET);
  } else
    fseek (mp3->fh, 0, SEEK_SET);

  /*                                      */

  mp3->vbr = 0;

  return find_first_frame (mp3);
}

static int mp3_scan (struct mp3_file *mp3) {
  int header;

  int frames = 0;
  int last_bitrate = -1;
  double total_bitrate = 0.0;
  double total_framesize = 0.0;

  size_t bitrate, samplerate;
  double frame_size;

  while (ftell (mp3->fh) < mp3->data_size) {
    fread (&header, 4, 1, mp3->fh);

    bitrate = bitrate_table [((header & 0x0000f000) >> 12)];
    samplerate = samplerate_table [(header & 0x00000c00) >> 10];

    if ((header & 0xffea0000) != 0xffea0000) {
      frames = 0;

      if (find_first_frame (mp3) < 0)
	return -1;
      continue;
    }

    last_bitrate = bitrate;
    total_bitrate += (double)bitrate;

    frame_size = 144000.0 * (double)bitrate/(double)samplerate + (double)((header & 0x00000200) >> 9);
    total_framesize += frame_size;
    fseek (mp3->fh, frame_size - 4, SEEK_CUR);

    frames++;

    if (frames == 4 && mp3->vbr == 0) {
      total_framesize = (double)(mp3->data_size - mp3->tagv2_size - mp3->skippage);

      total_bitrate = (double)bitrate;
      frames = 1;
      break;
    }
  }

  mp3->samplerate = samplerate;
  mp3->bitrate    = (int)(total_bitrate/(double)frames * 1000.0);
  mp3->length     = (int)(1000.0 * (total_framesize)/(total_bitrate/frames * 125.0));

  if (mp3->samplerate <= 0 || mp3->bitrate <= 0 || mp3->length <= 0)
    return -1;

  return 0;
}

void mp3_close (struct mp3_file *mp3) {
  fclose (mp3->fh);
}

int get_mp3_info (char *file_name, tihm_t *tihm) {
  struct mp3_file mp3;

  if (mp3_open (file_name, &mp3) < 0)
    return -1;

  mp3_scan (&mp3);
  mp3_close (&mp3);

  tihm->bitrate = mp3.bitrate/1000;
  tihm->vbr     = mp3.vbr;
  tihm->samplerate = mp3.samplerate;
  tihm->time       = mp3.length;
  tihm->size       = mp3.file_size;

  return 0;
}

/*
  mp3_fill_tihm:

  fills a tihm structure for adding a mp3 to the iTunesDB.

  Returns:
   < 0 if any error occured
     0 if successful
*/
int mp3_fill_tihm (u_int8_t *file_name, tihm_t *tihm){
  int ret;
  FILE *fh;

  u_int8_t type_string[] = "MPEG audio file";

  if (get_mp3_info(file_name, tihm) < 0) {
    return -1;
  }

  if ((fh = fopen(file_name,"r")) == NULL )
    return errno;

  if (get_id3_info(fh, file_name, tihm) < 0) {
    fclose (fh);

    return -1;
  }

  fclose (fh);

  dohm_add (tihm, type_string, strlen (type_string), "UTF-8", IPOD_TYPE);

  return 0;
}
