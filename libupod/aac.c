/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.5.0 aac.c
 *
 *   Parses Quicktime AAC files for bitrate, samplerate, etc.
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
#include "genre.h"

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <sys/stat.h>

#include <math.h>
#include <string.h>


unsigned int crc32 (unsigned char *, unsigned int);
void mp3_debug (char *, ...);

u_int16_t big16_2_arch16 (u_int16_t x) {
  u_int16_t z = x;

#if BYTE_ORDER==LITTLE_ENDIAN
  char *tmp = (char *)&z;
  char *tmpi = (char *)&x;

  tmp[0] = tmpi[1];
  tmp[1] = tmpi[0];
#endif

  return z;
}

u_int32_t big32_2_arch32 (u_int32_t x) {
  u_int32_t z = x;

#if BYTE_ORDER==LITTLE_ENDIAN
  char *tmp = (char *)&z;
  char *tmpi = (char *)&x;

  tmp[0] = tmpi[3];
  tmp[1] = tmpi[2];
  tmp[2] = tmpi[1];
  tmp[3] = tmpi[0];
#endif

  return z;
}

struct qt_atom {
  long size;
  long type;
};

struct qt_meta {
  u_int32_t offset;

  u_int8_t  flag;
  u_int8_t  identifier[3];

  u_int32_t unk0;
  u_int32_t datastr; /* "data" */

  u_int16_t data[4];
};

int is_container (int type) {
  if (type == string_to_int ("trak") ||
      type == string_to_int ("mdia") ||
      type == string_to_int ("moov") ||
      type == string_to_int ("stbl") ||
      type == string_to_int ("minf") ||
      type == string_to_int ("dinf") ||
      type == string_to_int ("udta"))
    return 1;

  return 0;
}

/* media header */
struct mdhd {
  u_int32_t flags;
  u_int32_t created;
  u_int32_t modified;
  u_int32_t time_scale;
  u_int32_t duration;
  u_int16_t language;
  u_int16_t quality;
};

int is_media_header (int type) {
  if (type == string_to_int ("mdhd"))
    return 1;

  return 0;
}

int parse_covr (char *buffer, int buffer_size, FILE *fd, struct qt_meta meta, tihm_t *tihm) {
  unsigned char *image_data;
  unsigned long cksum;

  if (tihm->image_data)
    return 0;

  mp3_debug ("Cover artwork found. Image size is %i B\n", meta.offset);

  image_data = (unsigned char *)calloc(1, meta.offset - sizeof (struct qt_meta));

  fseek (fd, - (meta.offset - sizeof (struct qt_meta)), SEEK_CUR);
  fread (image_data, 1, meta.offset - sizeof (struct qt_meta), fd);

  /* TODO -- Use a 64 bit checksum */
  cksum = crc32 (image_data, meta.offset - sizeof (struct qt_meta));

  tihm->has_artwork = 1;

  /* By using a checksum duplicate artwork will be avoided */
  tihm->artwork_id1 = cksum;

  tihm->image_data  = image_data;
  tihm->image_size  = meta.offset - sizeof (struct qt_meta);

  return 0;
}

/* Parse user data's (udat) meta data (meta) section */
int parse_meta (char *buffer, int buffer_size, FILE *fd, struct qt_atom atom, tihm_t *tihm) {
  int seeked = 46;
  
  /* skip 46 bytes of meta section (bytes before start of data -- I think) */
  fseek (fd, 46, SEEK_CUR);
	  
  while (1) {
    struct qt_meta meta;
    int data_type = -1;
    int size;
    
    fread (&meta, sizeof(struct qt_meta), 1, fd);
    
    seeked += meta.offset;

    size = meta.offset - sizeof(struct qt_meta);

    memset (buffer, 0, buffer_size);
    
    if (size > buffer_size || strncmp(meta.identifier, "ree",3) == 0)
      /* it is unlikely that any data we want will be larger than the buffer */
      fseek (fd, size, SEEK_CUR);
    else {
      fread (buffer, meta.offset - sizeof(struct qt_meta), 1, fd);
    
      buffer[meta.offset - sizeof(struct qt_meta)] = '\0';
    }

    /* some of the tag markers are 4 characters... i dont know how to correctly
     parse the meta data to check which (3 or 4 chars) we are encontering so
     handle all tag markers like they are only 3 chars (last 3 chars of 4 ones) */
    if (strncmp (meta.identifier, "ree", 3) == 0)
      /* catch the free atom to exit */
      break;
    else if (strncmp (meta.identifier, "nam", 3) == 0)
      data_type = IPOD_TITLE;
    else if (strncmp (meta.identifier, "ART", 3) == 0)
      data_type = IPOD_ARTIST;
    else if (strncmp (meta.identifier, "alb", 3) == 0)
      data_type = IPOD_ALBUM;
    else if (strncmp (meta.identifier, "nre", 3) == 0) {
      int genre_num = *((short *)buffer) - 1;
      data_type = IPOD_GENRE;

      dohm_add(tihm, genre_table[genre_num],
	       strlen(genre_table[genre_num]), "UTF-8", data_type);

      continue;
    } else if (strncmp (meta.identifier, "cmt", 3) == 0) 
      data_type = IPOD_COMMENT;
    else if (strncmp (meta.identifier, "gen", 3) == 0)
      data_type = IPOD_GENRE;
    else if (strncmp (meta.identifier, "rkn", 3) == 0) {
      tihm->track        = big16_2_arch16( ((short *)buffer)[1] );
      tihm->album_tracks = big16_2_arch16( ((short *)buffer)[2] );
      continue;
    } else if (strncmp (meta.identifier, "isk", 3) == 0) {
      tihm->disk_num   = big16_2_arch16( ((short *)buffer)[1] );
      tihm->disk_total = big16_2_arch16( ((short *)buffer)[2] );
    } else if (strncmp (meta.identifier, "day", 3) == 0) {
      tihm->year = strtol (buffer, NULL, 10);
      continue;
    } else if (strncmp (meta.identifier, "ovr", 3) == 0) {
      parse_covr (buffer, buffer_size, fd, meta, tihm);
      continue;
    } else if (strncmp (meta.identifier, "mpo", 3) == 0) {
      tihm->bpm = big16_2_arch16 ( ((short *)buffer)[0] );
      continue;
    } else
      continue;

    dohm_add(tihm, buffer, meta.offset - sizeof(struct qt_meta), "UTF-8", data_type);
  }

  fseek (fd, atom.size - seeked - 8, SEEK_CUR);

  return 0;
}

int m4a_bitrates[] = {
  28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1
};

void parse_stsz (FILE *fh, double *bits, int time_scale, int faac) {
  int current_loc = ftell (fh);
  int buffer[10];
  double avg = 0.0;
  int i;
  double bmax = 0.0;
  double bmin = 10000.0;
  int silence_frames = 0;
  double totalby;
  int num_samples = 0;
  long int bit_rate;

  mp3_debug ("Parsing stsz atom\n");

  fread (buffer, 4, 3, fh);

  num_samples = big32_2_arch32 (buffer[2]);

  mp3_debug ("Atom contains %i samples\n", num_samples);

  for (i = 0 ; i < num_samples ; i++) {
    int sample_size;

    fread (buffer, 4, 1, fh);

    sample_size = big32_2_arch32 (buffer[0]);

    if (sample_size > 7)
      avg += (double)sample_size;
    else
      silence_frames++;

    bmax = ((double)sample_size > bmax) ? (double)sample_size : bmax;
    bmin = ((double)sample_size < bmin) ? (double)sample_size : bmin;
  }

  bit_rate = (long int)(((avg * 8.0/((double)(num_samples-silence_frames))) * time_scale/1000.0)/1000.0);

  mp3_debug ("Bit_rate is: %i\n", bit_rate);

  /* I have no idea why this works for apple lossless (or if it works in all cases) */
  if ((bit_rate - 32) > m4a_bitrates[14])
    avg /= 4.096;


  mp3_debug ("Total sample size = %f\n", totalby=avg);

  avg /= (double)num_samples;

  mp3_debug ("Average sample is %f bits\n", avg);
  
  mp3_debug ("Minimum bitrate is: %f bps\n", bmin * 44.10 * 8.0);
  mp3_debug ("Agerage bitrate is: %f bps\n", avg * 44.10 * 8.0);
  mp3_debug ("Maximum bitrate is: %f bps\n", bmax * 44.10 * 8.0);

  *bits = totalby/((double)(num_samples-silence_frames)) * 8.0;

  fseek (fh, current_loc, SEEK_SET);
}

int aac_fill_tihm (char *file_name, tihm_t *tihm) {
  struct stat statinfo;
  int ret;

  char aac_type_string[] = "AAC audio file";
  char lossless_type_string[] = "Apple Lossless audio file";
  FILE *fd;
  int buffer_size = 2000;
  char buffer[buffer_size];

  int i;
  long int time_scale = 0, bit_rate;
  struct qt_atom atom;

  double duration = 0.0;

  int meta = string_to_int ("meta");
  int mdat = string_to_int ("mdat");
  int stsz = string_to_int ("stsz");
  int faac = 0;
  double bits;

  memset (tihm, 0, sizeof(tihm_t));

  ret = stat(file_name, &statinfo);

  if (ret < 0) {
    perror("aac_fill_tihm");
    return -errno;
  }

  tihm->size     = statinfo.st_size;
  tihm->mod_date = statinfo.st_mtimespec.tv_sec;
  tihm->creation_date = statinfo.st_mtimespec.tv_sec;

  if ((fd = fopen (file_name, "r")) == NULL)
    return -errno;

  fread (&atom, sizeof(atom), 1, fd);

  /* Check for ID3 tags */
  if (strncmp ((char *)&atom, "ID3", 3) == 0) {
    fseek (fd, 0, SEEK_SET);
    get_id3_info (fd, file_name, tihm);

    fread (&atom, sizeof(atom), 1, fd);
  } 

  /* Some AAC files don't use a quicktime header. Does the iPod even support
   them? For now upod only supports Quicktime AAC files */
  if (atom.type != string_to_int ("ftyp")) {
    fclose (fd);
    return -1;
  }

  fread (buffer, 1, atom.size - sizeof(atom), fd);
  if (strncmp (buffer, "M4A", 3) != 0 &&
      strncmp (buffer, "mp42", 4) != 0) {
    fclose (fd);

    return -1;
  }

  if (strncmp (buffer, "mp42", 4) == 0)
    faac = 1;

  while (1) {
    if (fread (&atom, sizeof(atom), 1, fd) != 1) {
      fclose (fd);
      return -errno;
    }

    if (atom.type == mdat || atom.size == 0)
      break;

    if (atom.type == stsz && atom.size > 0x14)
      parse_stsz (fd, &bits, time_scale, faac);

    if (atom.size > sizeof(atom)) {
      if (atom.size - sizeof(atom) < 2001) {
	if (is_media_header (atom.type) && time_scale == 0) {
	  struct mdhd *mdhd = (struct mdhd *)buffer;
	  
	  fread (buffer, atom.size - sizeof(atom), 1, fd);

	  time_scale = mdhd->time_scale;

	  duration = (double)mdhd->duration/(double)time_scale;

	  mp3_debug ("aac_fill_tihm: time_scale = %i, duration = %fsecs\n", time_scale, duration);
	} else if (atom.type == meta)
	  parse_meta (buffer, buffer_size, fd, atom, tihm);
	else if (!is_container(atom.type))
	  fseek (fd, atom.size - sizeof(atom), SEEK_CUR);
      } else {
	if (atom.type == meta)
	  parse_meta (buffer, buffer_size, fd, atom, tihm);
	else if (!is_container(atom.type))
	  fseek (fd, atom.size - sizeof(atom), SEEK_CUR);
      }
    } else
      fseek (fd, atom.size - sizeof(atom), SEEK_CUR);
  }

  fseek (fd, 16, SEEK_CUR);

  bit_rate = (long int)((bits * (double)time_scale/1000.0)/1000.0);
  mp3_debug ("Best guess is     : %i kbps\n", bit_rate);

  mp3_debug ("aac_fill_tihm: size = %i\n", atom.size);

  if (!faac) {
    if ((bit_rate - 32) <= m4a_bitrates[14]) {
      for (i = 1 ; m4a_bitrates[i] > 0 ; i++) {
	int temp = m4a_bitrates[i-1] - bit_rate;
	int temp2 = m4a_bitrates[i] - bit_rate;
	if (temp < 0 && (temp + temp2) > 0) {
	  bit_rate = m4a_bitrates[i-1];
	  break;
	} else if (temp < 0 && temp2 > 0 && (temp + temp2) <= 0) {
	  bit_rate = m4a_bitrates[i];
	  break;
	}
      }
      dohm_add (tihm, aac_type_string, strlen (aac_type_string), "UTF-8", IPOD_TYPE);
    } else
      dohm_add (tihm, lossless_type_string, strlen (lossless_type_string), "UTF-8", IPOD_TYPE);
  }

  if (bit_rate == 0)
    return -1;

  tihm->time = lround (duration * 1000.0);
  tihm->samplerate = time_scale;
  tihm->bitrate = bit_rate;

  if (tihm->bitrate == 0) {
    fclose(fd);
    return -1;
  }

  fclose (fd);

  return 0;
}
