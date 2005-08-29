/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.6.0a aac.c
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

#include "itunesdbi.h"

#include <math.h>

struct m4a_file {
  FILE *fh;

  int file_size;  /* Bytes */
  int bitrate;    /* bps */
  int samplerate; /* samples/sec */
  int duration; /* ms */
  int mod_date;
  int faac;
  int apple_lossless;
  size_t meta_offset;
};

struct qt_atom {
  long size;
  long type;
};

struct qt_meta {
  u_int32_t offset;

  /* flag is sometimes the first character of the identifier */
  u_int8_t  flag;
  u_int8_t  identifier[3];

  u_int32_t unk0;
  u_int32_t datastr; /* "data" */

  u_int16_t data[4];
};

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

static int m4a_bitrates[] = {
  28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1
};

#define BITRATE(sample_size, num_samples, time_scale) ((sample_size * 8.0)/(double)(num_samples) * (double)time_scale/1000.0)/1000.0

#define is_media_header(type) (type == string_to_int ("mdhd"))
/* this list (like this parser) is only as complete as it needs to be */
#define is_container(type) (type == string_to_int ("trak") || \
			    type == string_to_int ("mdia") || \
			    type == string_to_int ("moov") || \
			    type == string_to_int ("stbl") || \
			    type == string_to_int ("minf") || \
			    type == string_to_int ("dinf") || \
			    type == string_to_int ("udta"))

#if defined(HAVE_LIBWAND)
int read_artwork (FILE *fh, tihm_t *tihm, size_t image_size);
#endif

static int parse_covr (FILE *fh, struct qt_meta meta, tihm_t *tihm) {
#if defined(HAVE_LIBWAND)
  if (tihm->image_data)
    return 0;

  fseek (fh, - (meta.offset - sizeof (struct qt_meta)), SEEK_CUR);

  return read_artwork (fh, tihm, meta.offset - sizeof (struct qt_meta));
#else
  mp3_debug ("Cover artwork found and ignored (libupod compiled without libwand).\n");

  return 0;
#endif  
}

/* Parse user data atom's (udat) meta data (meta) */
int aac_parse_meta (struct m4a_file *aac, tihm_t *tihm) {
  int seeked = 46;
  struct qt_meta meta;
  int size;
  int buffer_size = 512;
  unsigned char buffer[buffer_size];
  
  if (aac->meta_offset == 0)
    return;

  fseek (aac->fh, aac->meta_offset + 4, SEEK_SET);
  fread (&meta, sizeof(struct qt_meta), 1, aac->fh);
  size = meta.offset - sizeof(struct qt_meta);
  
  fseek (aac->fh, size + 8, SEEK_CUR);

  while (1) {
    int data_type = -1;
    
    fread (&meta, sizeof(struct qt_meta), 1, aac->fh);
    
    seeked += meta.offset;

    size = meta.offset - sizeof(struct qt_meta);

    memset (buffer, 0, buffer_size);

    mp3_debug ("Meta identifier: %c%c%c\n", meta.identifier[0], meta.identifier[1], meta.identifier[2]);
    mp3_debug ("Meta offset: %i\n", meta.offset);
    
    if (size > buffer_size || strncmp((char *)&meta.flag, "free", 4) == 0)
      /* it is unlikely that any data we want will be larger than the buffer */
      fseek (aac->fh, size, SEEK_CUR);
    else {
      fread (buffer, meta.offset - sizeof(struct qt_meta), 1, aac->fh);
    
      buffer[meta.offset - sizeof(struct qt_meta)] = '\0';
    }

    /* some of the tag markers are 4 characters... i dont know how to correctly
     parse the meta data to check which (3 or 4 chars) we are encontering so
     handle all tag markers like they are only 3 chars (last 3 chars of 4 ones) */
    if (strncmp ((char *)&meta.flag, "free", 4) == 0)
      /* catch the free atom to exit */
      break;
    else if (strncmp ((char *)meta.identifier, "nam", 3) == 0)
      data_type = IPOD_TITLE;
    else if (strncmp ((char *)meta.identifier, "ART", 3) == 0)
      data_type = IPOD_ARTIST;
    else if (strncmp ((char *)meta.identifier, "alb", 3) == 0)
      data_type = IPOD_ALBUM;
    else if (strncmp ((char *)&meta.flag, "gnre", 4) == 0) {
      int genre_num = *((short *)buffer) - 1;
      data_type = IPOD_GENRE;

      dohm_add(tihm, genre_table[genre_num],
	       strlen(genre_table[genre_num]), "UTF-8", data_type);

      continue;
    } else if (strncmp ((char *)meta.identifier, "cmt", 3) == 0) 
      data_type = IPOD_COMMENT;
    else if (strncmp ((char *)meta.identifier, "gen", 3) == 0)
      data_type = IPOD_GENRE;
    else if (strncmp ((char *)&meta.flag, "trkn", 4) == 0) {
      tihm->track        = big16_2_arch16( ((short *)buffer)[1] );
      tihm->album_tracks = big16_2_arch16( ((short *)buffer)[2] );
      continue;
    } else if (strncmp ((char *)&meta.flag, "disk", 4) == 0) {
      tihm->disk_num   = big16_2_arch16( ((short *)buffer)[1] );
      tihm->disk_total = big16_2_arch16( ((short *)buffer)[2] );
    } else if (strncmp ((char *)meta.identifier, "day", 3) == 0) {
      tihm->year = strtol ((char *)buffer, NULL, 10);
      continue;
    } else if (strncmp ((char *)&meta.flag, "covr", 4) == 0) {
      parse_covr (aac->fh, meta, tihm);
      continue;
    } else if (strncmp ((char *)&meta.flag, "tmpo", 4) == 0) {
      tihm->bpm = big16_2_arch16 ( ((short *)buffer)[0] );
      continue;
    } else
      continue;

    dohm_add(tihm, (char *)buffer, meta.offset - sizeof(struct qt_meta), "UTF-8", data_type);
  }

  return 0;
}

/* stsz atoms contain a table of sample sizes */
static void parse_stsz (FILE *fh, unsigned int *bit_rate, int *lossless, int time_scale) {
  int current_loc = ftell (fh);
  unsigned int buffer[3];
  double avg = 0.0;
  double bmax = 0.0;
  double bmin = 10000.0;
  int silent_samples = 0;
  int num_samples = 0;

  int i;

  mp3_debug ("Parsing stsz atom\n");

  fread (buffer, 4, 3, fh);

  num_samples = big32_2_arch32 (buffer[2]);

  mp3_debug ("Atom contains %i samples\n", num_samples);

  for (i = 0 ; i < num_samples ; i++) {
    int sample_size;

    fread (buffer, 4, 1, fh);

    sample_size = big32_2_arch32 (buffer[0]);

    /* Silent samples are 7 bytes in size */
    if (sample_size == 7)
      silent_samples++;
    else
      avg += (double)sample_size;

    bmax = fmax ((double)sample_size, bmax);
    bmin = fmin ((double)sample_size, bmin);
  }
  
  /* Silent frames throw off the average bitrate calculation for iTunes-created AAC files
     and are thus dropped from the calculated bitrate. */
  *bit_rate = (unsigned int)BITRATE(avg, num_samples-silent_samples, time_scale);
  
  /* I have no idea why this works for apple lossless (or if it works in all cases) */
  if ((*bit_rate - 32) > m4a_bitrates[14]) {
    bmin *= 1000.0/4096.0;
    avg  *= 1000.0/4096.0;
    bmax *= 1000.0/4096.0;

    *lossless = 1;
    *bit_rate = (unsigned int)BITRATE(avg, num_samples-silent_samples, time_scale);
  }

  avg /= (double)(num_samples-silent_samples);
  
  mp3_debug ("Minimum bitrate is: %f kbps\n", BITRATE(bmin, 1, time_scale));
  mp3_debug ("Agerage bitrate is: %f kbps\n", BITRATE(avg, 1, time_scale));
  mp3_debug ("Maximum bitrate is: %f kbps\n", BITRATE(bmax, 1, time_scale));

  fseek (fh, current_loc, SEEK_SET);
}



int aac_open (char *file_name, struct m4a_file *aac) {
  struct qt_atom atom;
  struct stat statinfo;
  char buffer[4];

  mp3_debug ("aac_open: entering\n");

  memset (aac, 0, sizeof(struct m4a_file));

  if (stat(file_name, &statinfo) < 0)
    return -errno;

  aac->file_size = statinfo.st_size;
  aac->mod_date  = statinfo.st_mtime;

  if ((aac->fh = fopen (file_name, "r")) == NULL)
    return -errno;

  fread (&atom, sizeof(atom), 1, aac->fh);

  /* Some AAC files don't use a quicktime header. Does the iPod even support
   them? For now upod only supports Quicktime AAC files */
  if (atom.type != string_to_int ("ftyp")) {
    fclose (aac->fh);
    return -1;
  }

  fread (buffer, 1, 4, aac->fh);
  if (strncmp (buffer, "M4A", 3) != 0 &&
      strncmp (buffer, "mp42", 4) != 0) {
    fclose (aac->fh);

    return -1;
  }

  fseek (aac->fh, atom.size - sizeof(atom) - 4, SEEK_CUR);

  if (strncmp (buffer, "mp42", 4) == 0)
    aac->faac = 1;

  return 0;
}

int aac_scan (struct m4a_file *aac) {
  struct qt_atom atom;
  int meta = string_to_int ("meta");
  int mdat = string_to_int ("mdat");
  int stsz = string_to_int ("stsz");
  int i;

  while (1) {
    if (fread (&atom, sizeof(atom), 1, aac->fh) != 1) {
      mp3_debug ("aac_scan: could not read atom header.\n");
      break;
    }

    /* Stop parsing on media data atom (audio data follows) */
    if (atom.type == mdat || atom.size == 0)
      break;

    /* I don't know what it means when an AAC has multiple stsz atoms, so
       only use the first encountered that has a bit_rate average over 0. */
    if (atom.type == stsz && atom.size > 0x14 && aac->bitrate == 0)
      parse_stsz (aac->fh, (unsigned int *)&aac->bitrate, &aac->apple_lossless, aac->samplerate);

    if (!is_container(atom.type)) {
      /* I have come across AAC files that contain multiple media headers. When this
	 occurs, all but the first with a non-zero time_scale will be ignored. */
      if (is_media_header (atom.type) && aac->samplerate == 0) {
	struct mdhd mdhd;
	
	fread (&mdhd, sizeof (struct mdhd), 1, aac->fh);
	fseek (aac->fh, atom.size - sizeof(atom) - sizeof (struct mdhd), SEEK_CUR);

	aac->samplerate = mdhd.time_scale;
	aac->duration = lround (1000.0 * (double)mdhd.duration/(double)aac->samplerate);

	mp3_debug ("aac_fill_tihm: time_scale = %i, duration = %f seconds.\n",
		   aac->samplerate, aac->duration);
      } else if (atom.type == meta) {
	aac->meta_offset = ftell (aac->fh);
	fseek (aac->fh, atom.size - sizeof(atom), SEEK_CUR);
      } else
	fseek (aac->fh, atom.size - sizeof(atom), SEEK_CUR);
    }
  }

  if (!aac->faac && !aac->apple_lossless) {
    for (i = 1 ; m4a_bitrates[i] > 0 ; i++) {
      int temp = m4a_bitrates[i-1] - aac->bitrate;
      int temp2 = m4a_bitrates[i] - aac->bitrate;

      if (temp <= 0 && (temp + temp2) > 0)
	break;
    }

    aac->bitrate = m4a_bitrates[i-1];
  }

  if (aac->bitrate <= 0)
    return -1;

  return 0;
}

void aac_close (struct m4a_file *aac) {
  if (aac && aac->fh) {
    fclose (aac->fh);
    aac->fh = NULL;
  }
}

int aac_fill_tihm (char *file_name, tihm_t *tihm) {
  char aac_type_string[] = "AAC audio file";
  char lossless_type_string[] = "Apple Lossless audio file";

  struct m4a_file aac;

  if (aac_open (file_name, &aac) < 0)
    return -1;

  aac_scan (&aac);
  aac_parse_meta (&aac, tihm);

  aac_close (&aac);

  memset (tihm, 0, sizeof (tihm_t));

  tihm->time          = aac.duration;
  tihm->samplerate    = aac.samplerate;
  tihm->bitrate       = aac.bitrate;
  tihm->time          = aac.duration;
  tihm->size          = aac.file_size;
  tihm->mod_date      = aac.mod_date;
  tihm->creation_date = aac.mod_date;

  if (!aac.apple_lossless)
    dohm_add (tihm, aac_type_string, strlen (aac_type_string), "UTF-8", IPOD_TYPE);
  else
    dohm_add (tihm, lossless_type_string, strlen (lossless_type_string), "UTF-8", IPOD_TYPE);

  if (tihm->bitrate == 0)
    return -1;

  return 0;
}
