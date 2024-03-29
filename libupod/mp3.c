/**
 *   (c) 2002-2004 Nathan Hjelm <hjelmn@cs.unm.edu>
 *   v0.3.3 mp3.c 
 *
 *   (2004-08-26) : Fixed mp3 parsing for different samplerates.
 *   (2004-10-28) : Correctly identifies proper mpeg frame headers.
 *   (2004-10-28) : Correctly parses files with LYRICS tags.
 *
 *   ID3/LYRIC Tag information can be found at http://www.id3.org.
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

#include "itunesdbi.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdarg.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#define MP3_PROTECTION_BIT 0x00010000
#define MP3_PADDING_BIT    0x00000200

/* control the number of frames used by mp3_scan in approximating a file's bitrate and duration */
#define FRAME_COUNT 30 /* 30 frames appears to be a reasonable trade-off between speed and accuracy */

#if defined(MP3_DEBUG)
void mp3_debug (char *format, ...) {
  va_list arg;
  va_start (arg, format);
  vfprintf (stderr, format, arg);
  va_end (arg);
}
#endif

struct mp3_file {
  FILE *fh;

  int file_size;  /* Bytes */
  int tagv2_size; /* Bytes */
  int skippage;   /* Bytes */
  int data_size;  /* Bytes */

  int vbr;
  int bitrate;    /* bps */

  int initial_header;

  int frames;
  long long samples;
  int xdata_size;

  int layer_index;
  int version_index;

  int samplerate; /* samples/sec */

  int duration; /* ms */
  int mod_date;
};

/* [version][layer][bitrate] */
int bitrate_table[4][4][16] = {
  /* v2.5 */
  {{-1, -1, -1, -1, -1. -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1},
   {-1,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, -1},
   {-1,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, -1},
   {-1, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1}},

  /* NOTUSED */
  {{-1, -1, -1, -1, -1. -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1},
   {-1, -1, -1, -1, -1. -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1},
   {-1, -1, -1, -1, -1. -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1},
   {-1, -1, -1, -1, -1. -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1}},

  /* v2 */
  {{-1, -1, -1, -1, -1. -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1},
   {-1,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, -1},
   {-1,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, -1},
   {-1, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1}},

  /* v1 */
  {{-1, -1, -1, -1,  -1.  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1},
   {-1, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, -1},
   {-1, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, -1},
   {-1, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1}},

};

int samplerate_table[4][4] = {
  {11025, 12000,  8000, -1},
  {   -1,    -1,    -1, -1},
  {22050, 24000, 16000, -1},
  {44100, 48000, 32000, -1}
};

/* samples_per_frame[version_index][layer_index] */
int samples_per_frame[4][4] = {
  {-1,  576, 1152, 384}, /*    v2.5 */
  {-1,   -1,   -1,  -1}, /* NOTUSED */
  {-1,  576, 1152, 384}, /*      v2 */
  {-1, 1152, 1152, 384}  /*      v1 */
};

double version_table[] = {
  2.5, -1.0, 2.0, 1.0
};

size_t layer_table[] = {
  -1, 3, 2, 1
};

/* in id3.c */
int id3v2_size (unsigned char buf[14]);

#define MPEG_VERSION(header) ((header & 0x00180000) >> 19)
#define MPEG_LAYER(header) ((header & 0x00060000) >> 17)
#define MPEG_BITRATEI(header) ((header & 0x0000f000) >> 12)
#define MPEG_SAMPLERATEI(header) ((header & 0x00000c00) >> 10)
#define MPEG_PADDING(header) ((header & 0x00000200) >> 9)

#define BITRATE(header) bitrate_table[MPEG_VERSION(header)][MPEG_LAYER(header)][MPEG_BITRATEI(header)]
#define SAMPLERATE(header) samplerate_table[MPEG_VERSION(header)][MPEG_SAMPLERATEI(header)]
#define PADDING(header) ((MPEG_LAYER(header) == 0x3) ? 4 : 1)

static size_t mpeg_frame_length (int header) {
  double bitrate = (double)BITRATE(header) * 1000.0;
  double samplerate = (double)SAMPLERATE(header);
  double padding = (double)MPEG_PADDING(header);
  char layer = MPEG_LAYER(header);
  size_t frame_length;

  if (layer == 0x11)
    frame_length = 144.0 * bitrate/samplerate + padding;
  else
    frame_length = (12 * bitrate/samplerate + padding) * 4.0;

  return (size_t)((layer != 0x11) ? (144.0 * bitrate/samplerate + padding)
	  : (12 * bitrate/samplerate + padding) * 4.0);
}

/* check_mp3_header: returns 0 on success */
static int check_mp3_header (int header) {
  if (((header & 0xffe00000) == 0xffe00000) &&
      (MPEG_VERSION(header) > 0.0) && (BITRATE(header) > 0)
      && (SAMPLERATE(header) > 0))
    return 0;
  else if (header == 0x4d4c4c54) /* MLLT */
    return 2;
  else
    return 1;
}

static int find_first_frame (struct mp3_file *mp3) {
  int header;
  int buffer;
  int ret;
  mp3->skippage = 0;

  while (fread (&header, 4, 1, mp3->fh)) {
    header = big32_2_arch32 (header);

    /* MPEG-1 Layer III */
    if ((ret = check_mp3_header (header)) == 0) {
      /* Check for an Xing frame (speeds up parsing) */
      fseek (mp3->fh, 32, SEEK_CUR);
      fread (&buffer, 4, 1, mp3->fh);

      if (buffer == string_to_int ("Xing")) {
	int xstart = ftell (mp3->fh);
	int xflags;

	/* an mp3 with an Xing header is ALWAYS vbr */
	mp3->vbr = 1;

	fread (&xflags, 4, 1, mp3->fh);

	mp3_debug ("Xing flags = %08x\n", xflags);

	if (xflags & 0x00000001) {
	  fread (&buffer, 4, 1, mp3->fh);
	  mp3->frames = buffer;

	  mp3_debug ("MPEG file has %i frames\n", mp3->frames);
	}

	if (xflags & 0x00000002) {
	  fread (&buffer, 4, 1, mp3->fh);
	  mp3->xdata_size = buffer;

	  mp3_debug ("MPEG file has %i bytes of data\n", mp3->xdata_size);
	}

	fseek (mp3->fh, xstart, SEEK_SET);
      }

      mp3->initial_header = header;
      mp3->samplerate  = SAMPLERATE(header);
      mp3->layer_index = MPEG_LAYER(header);
      mp3->version_index = MPEG_VERSION(header);

      mp3_debug ("Inital bitrate = %i\n", BITRATE(header));

      fseek (mp3->fh, -40, SEEK_CUR);
      return 0;
    } else if (ret == 2)
      return -2;
    
    fseek (mp3->fh, -3, SEEK_CUR);
    mp3->skippage++;
  }

  return -1;
}

static int mp3_open (char *file_name, struct mp3_file *mp3) {
  struct stat statinfo;

  unsigned char buffer[14];
  int has_v1 = 0;

  mp3_debug ("mp3_open: Entering...\n");

  memset (mp3, 0 , sizeof (struct mp3_file));

  if (stat (file_name, &statinfo) < 0)
    return -errno;

  mp3->file_size = mp3->data_size = statinfo.st_size;
  mp3->mod_date  = statinfo.st_mtime;

  mp3->fh = fopen (file_name, "r");
  if (mp3->fh == NULL) 
    return -errno;

  /* Adjust total_size if an id3v1 tag exists */
  fseek (mp3->fh, -128, SEEK_END);
  memset (buffer, 0, 5);

  fread (buffer, 1, 3, mp3->fh);
  if (strncmp ((char *)buffer, "TAG", 3) == 0) {
    mp3->data_size -= 128;

    has_v1 = 1;

    mp3_debug ("mp3_open: Found id3v1 tag.\n");
  }
  /*                                          */

  /* Check for Lyrics v2.00 */
  fseek (mp3->fh, -9 - (has_v1 ? 128 : 0), SEEK_END);
  memset (buffer, 0, 10);
  fread (buffer, 1, 9, mp3->fh);

  if (strncmp ((char *)buffer, "LYRICS200", 9) == 0) {
    int lyrics_size;
    mp3_debug ("mp3_open: Found Lyrics v2.00\n");

    /* Get the size of the Lyrics */
    fseek (mp3->fh, -15, SEEK_CUR);
    memset (buffer, 0, 7);
    fread (buffer, 1, 6, mp3->fh);

    /* Include the size if LYRICS200 (9) and the size field (6) */
    lyrics_size = strtol ((char *)buffer, NULL, 10) + 15;
    mp3->data_size -= lyrics_size;

    mp3_debug ("mp3_open: Lyrics are 0x%x Bytes in length.\n", lyrics_size);
  }

  /* find and skip id3v2 tag if it exists */
  fseek (mp3->fh, 0, SEEK_SET);
  fread (buffer, 1, 14, mp3->fh);    
  mp3->tagv2_size = id3v2_size (buffer);

  fseek (mp3->fh, mp3->tagv2_size, SEEK_SET);

  mp3_debug ("mp3_open: id3v2 size: 0x%08x\n", mp3->tagv2_size);
  /****************************************/

  mp3->vbr = 0;

  mp3_debug ("mp3_open: Complete\n");

  return find_first_frame (mp3);
}

static int mp3_scan (struct mp3_file *mp3) {
  int header;
  int ret;
  int frames = 0;
  int last_bitrate = -1;
  int total_framesize = 0;

  size_t bitrate;
  int frame_size;

  mp3_debug ("mp3_scan: Entering...\n");

  if (mp3->frames == 0 || mp3->xdata_size == 0) {
    /* This calculation will (from time to time) produce a duration that does not agree with the
       value produced by itunes. */
    while (ftell (mp3->fh) < mp3->data_size && (frames < FRAME_COUNT || mp3->vbr)) {
      fread (&header, 4, 1, mp3->fh);

      header = big32_2_arch32 (header);
      
      if (check_mp3_header (header) != 0) {
	fseek (mp3->fh, -4, SEEK_CUR);

	mp3_debug ("mp3_scan: Invalid header %08x %08x Bytes into the file.\n", header, ftell(mp3->fh));
	
	if ((ret = find_first_frame (mp3)) == -1) {
	  mp3_debug ("mp3_scan: An error occured at line: %i\n", __LINE__);
	  
	  /* This is hack-ish, but there might be junk at the end of the file. */
	  
	  break;
	} else if (ret == -2) {
	  mp3_debug ("mp3_scan: Ran into MLLT frame.\n");
	  
	  mp3->data_size -= (mp3->file_size) - ftell (mp3->fh);
	  
	  break;
	}
	
	continue;
      }
      
      bitrate = BITRATE(header);
      
      if (!mp3->vbr && (last_bitrate != -1) && (bitrate != last_bitrate))
	mp3->vbr = 1;
      else
	last_bitrate = bitrate;
      
      frame_size = mpeg_frame_length (header);
      total_framesize += frame_size;
      fseek (mp3->fh, frame_size - 4, SEEK_CUR);      
      frames++;
    }

    /* approximate the number of frames in the file */
    if (frames == FRAME_COUNT) {
      frames = (int)((double)((mp3->data_size - mp3->tagv2_size) * FRAME_COUNT) / (double)total_framesize);
      total_framesize = mp3->data_size - mp3->tagv2_size;
    }

    if (mp3->frames == 0)
      mp3->frames = frames;

    if (mp3->xdata_size == 0)
      mp3->xdata_size = total_framesize;
  }

  /* duration (ms) = frames * ms/sec * (samples/frame)/(samples/sec) */
  mp3->samples    = (long long)samples_per_frame[mp3->version_index][mp3->layer_index] * (long long)mp3->frames;
  mp3->duration   = (int)(1000.0 * (double)mp3->samples/(double)mp3->samplerate);
  mp3->bitrate    = (int)(((float)mp3->xdata_size * 8.0)/(float)mp3->duration);

  mp3_debug ("mp3_scan: Finished scan. SampleRate: %i, BitRate: %i, Length: %i, Frames: %i.\n",
	     mp3->samplerate, mp3->bitrate, mp3->duration, mp3->frames);

  if (mp3->samplerate <= 0 || mp3->bitrate <= 0 || mp3->duration <= 0)
    return -1;

  return 0;
}

void mp3_close (struct mp3_file *mp3) {
  if (mp3->fh)
    fclose (mp3->fh);
}

int get_mp3_info (char *file_name, tihm_t *tihm) {
  struct mp3_file mp3;

  if (mp3_open (file_name, &mp3) < 0) {
    mp3_close (&mp3); /* Make sure everything is cleaned up */
    return -1;
  }

  if (mp3_scan (&mp3) < 0) {
    mp3_close (&mp3);
    return -1;
  }

  mp3_close (&mp3);

  tihm->bitrate = mp3.bitrate;
  tihm->vbr     = mp3.vbr;
  tihm->samplerate = mp3.samplerate;
  tihm->time       = mp3.duration;
  tihm->size       = mp3.file_size;
  tihm->mod_date   = mp3.mod_date;
  tihm->creation_date = mp3.mod_date;
  tihm->type      = string_to_int ("MP3 ");
  tihm->is_video  = 0;

  return 0;
}

/*
  mp3_fill_tihm:

  fills a tihm structure for adding a mp3 to the iTunesDB.

  Returns:
   < 0 if any error occured
     0 if successful
*/
int mp3_fill_tihm (char *file_name, tihm_t *tihm){
  FILE *fh;

  char type_string[] = "MPEG audio file";

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
