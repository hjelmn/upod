/**
 *   (c) 2003 Nathan Hjelm <hjelmn@unm.edu>
 *   v0.2 aac.c
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

struct qt_atom {
  long size;
  long type;
};

struct qt_meta {
  u_int32_t offset;

  u_int8_t  flag;
  u_int8_t  identifier[3];

  u_int16_t unk0;
  u_int16_t length;

  u_int32_t data;
  u_int32_t unk2;
  u_int32_t unk3;
};

int type_int (char type[4]) {
  return (((int)type[0] << 24) + ((int)type[1] << 16) + ((int)type[2] << 8) + (int)type[3]);
}

int is_container (int type) {
  if (type == type_int ("trak") ||
      type == type_int ("mdia") ||
      type == type_int ("moov") ||
      type == type_int ("stbl") ||
      type == type_int ("minf") ||
      type == type_int ("dinf") ||
      type == type_int ("udta"))
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
  if (type == type_int ("mdhd"))
    return 1;

  return 0;
}

int parse_meta (char *buffer, int buffer_size, FILE *fd, struct qt_atom atom, tihm_t *tihm) {
  int seeked = 46;
  
  fseek (fd, 46, SEEK_CUR);
	  
  while (1) {
    struct qt_meta meta;
    int data_type;
    int size;
    
    fread (&meta, sizeof(struct qt_meta), 1, fd);
    
    seeked += meta.offset;

    size = meta.offset - sizeof(struct qt_meta);
    
    if (size > buffer_size || strncmp(meta.identifier, "ree",3) == 0)
      /* it is unlikely that any data we want will be larger than the buffer */
      fseek (fd, size, SEEK_CUR);
    else {
      fread (buffer, meta.offset - sizeof(struct qt_meta), 1, fd);
    
      buffer[meta.offset - sizeof(struct qt_meta)] = '\0';
    }

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
      data_type = IPOD_GENRE;
      dohm_add(tihm, genre_table[*(short *)buffer - 1],
	       strlen(genre_table[*(short *)buffer - 1]), data_type);

      break;
    } else if (strncmp (meta.identifier, "cmt", 3) == 0) 
      data_type = IPOD_COMMENT;
    else if (strncmp (meta.identifier, "gen", 3) == 0)
      data_type = IPOD_GENRE;
    else
      continue;

    dohm_add(tihm, buffer, meta.offset - sizeof(struct qt_meta), data_type);
  }

  fseek (fd, atom.size - seeked - 8, SEEK_CUR);
}

int m4a_bitrates[] = {
  28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1
};

int aac_fill_tihm (char *file_name, tihm_t *tihm) {
  struct stat statinfo;
  int ret;

  char type_string[] = "AAC audio file";

  FILE *fd;
  int buffer_size = 2000;
  char buffer[buffer_size];

  int i;
  long long int duration, time_scale, bit_rate;
  struct qt_atom atom;

  memset (tihm, 0, sizeof(tihm_t));

  ret = stat(file_name, &statinfo);
  if (ret < 0) {
    perror("aac_fill_tihm");
    return -1;
  }

  tihm->size = statinfo.st_size;

  if ((fd = fopen (file_name, "r")) == NULL)
    return -1;

  fread (&atom, sizeof(atom), 1, fd);
  if (atom.type != type_int ("ftyp")) {
    fprintf (stderr, "File does not begin with ftyp Quicktime atom\n");

    fclose (fd);
    return -1;
  }

  fread (buffer, 1, atom.size - sizeof(atom), fd);
  if (strncmp (buffer, "M4A", 3) != 0) {
    fprintf (stderr, "File is not an MPEG4 file\n");

    fclose (fd);

    return -1;
  }

  while (1) {
    int meta = type_int ("meta");
    int mdat = type_int ("mdat");

    fread (&atom, sizeof(atom), 1, fd);

    if (atom.type == mdat)
      break;

    if (atom.size > sizeof(atom)) {
      if (atom.size - sizeof(atom) < 2001) {
	if (is_media_header (atom.type)) {
	  struct mdhd *mdhd = (struct mdhd *)buffer;
	  
	  fread (buffer, atom.size - sizeof(atom), 1, fd);

	  time_scale = mdhd->time_scale;
	  duration = mdhd->duration/time_scale;
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

  bit_rate = (atom.size / 128) / duration;

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

  tihm->time = duration * 1000;
  tihm->samplerate = time_scale;
  tihm->bitrate = bit_rate;

  dohm_add (tihm, type_string, strlen (type_string), IPOD_TYPE);
  fclose (fd);

  return 0;
}