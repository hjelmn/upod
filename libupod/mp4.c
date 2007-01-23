/**
 *   (c) 2003-2007 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.7.2 mp4.c
 *
 *   Parses MPEG-4 files
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
#include <time.h>

struct mp4_file {
  FILE *fh;

  int file_size;  /* Bytes */
  int bitrate;    /* bps */
  int samplerate; /* samples/sec */
  int duration; /* ms */
  int mtime, ctime;
  int faac;
  int apple_lossless;
  size_t meta_offset, meta_size;
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

struct tag_map {
  char mp4_tag[5];
  int ipod_tag;
};

struct tag_map tagmap[] = {{"nam" , IPOD_TITLE},
			   {"ART" , IPOD_ARTIST},
			   {"alb" , IPOD_ALBUM},
			   {"desc", IPOD_DESCRIPTION},
			   {"catg", IPOD_CATAGORY},
			   {"egid", IPOD_URL},
			   {"cmt" , IPOD_COMMENT},
			   {"gen" , IPOD_GENRE},
			   {"gnre", IPOD_GENRE},
			   {"purl", IPOD_PODCAST_URL},
			   {"", -1}};

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

  return read_artwork (fh, tihm, meta.offset - sizeof (struct qt_meta));
#else
  mp3_debug ("Cover artwork found and ignored (libupod compiled without libwand).\n");

  /* skip image data */
  fseek (fh, meta.offset - sizeof (struct qt_meta), SEEK_CUR);

  return 0;
#endif  
}


/*
  mp4_parse_meta:

  Parse meta data (meta) contained in a file's udat atom.
  This function must be called after mp4_scan.
*/
int mp4_parse_meta (struct mp4_file *mp4p, tihm_t *tihm) {
  struct qt_meta meta;
  struct qt_atom atom;
  size_t ilist_size, next_offset;
  int size, i, ret, data_type;
  int buffer_size = 512;
  unsigned char buffer[buffer_size];
  char meta_tag[5];
  
  if (mp4p->meta_offset == 0)
    return -1;

  fseek (mp4p->fh, mp4p->meta_offset + 4, SEEK_SET);

  /* read and skip hdlr atom */
  fread (&atom, sizeof (struct qt_atom), 1, mp4p->fh);
  if (atom.type != string_to_int ("hdlr"))
    return -1;

  fseek (mp4p->fh, atom.size - sizeof(struct qt_atom), SEEK_CUR);

  /* read and ilst atom */
  fread (&atom, 8, 1, mp4p->fh);  
  if (atom.type != string_to_int ("ilst"))
    /* no ilst == no tags */
    return -1;

  ilist_size  = atom.size;
  
  for (next_offset = 8 ; next_offset < ilist_size ; ) {
    /* read meta tag */
    ret = fread (&meta, sizeof(struct qt_meta), 1, mp4p->fh);
    if (ret != 1)
      break;

    memset (meta_tag, 0, 5);
    if (meta.flag == 0xa9)
      memcpy (meta_tag, meta.identifier, 3);
    else
      memcpy (meta_tag, &meta.flag, 4);

    size = meta.offset - sizeof(struct qt_meta);

    next_offset += meta.offset;

    mp3_debug ("mp4.c/mp4_parse_meta: Meta {identifier, offset} = {%-4s, 0x%08x}\n", meta_tag, meta.offset);
    mp3_debug ("mp4.c/mp4_parse_meta: ilst offset = %d/%d\n", (next_offset - meta.offset), ilist_size);

    if (strncmp (meta_tag, "covr", 4) == 0) {
      parse_covr (mp4p->fh, meta, tihm);

      continue;
    } else if (size > buffer_size) {
      /* it is unlikely that any data we want will be larger than the buffer */
      fseek (mp4p->fh, size, SEEK_CUR);

      continue;
    } else {
      if (size != buffer_size)
	buffer[size] = '\0';

      fread (buffer, 1, size, mp4p->fh);
    }

    /* look for a mapping from meta tag to ipod tag */
    for (i = 0, data_type = -1 ; tagmap[i].ipod_tag > -1 ; i++) 
      if (strcmp (meta_tag, tagmap[i].mp4_tag) == 0) {
	data_type = tagmap[i].ipod_tag;
	break;
      }

    if (data_type == IPOD_PODCAST_URL)
      tihm->is_podcast = 1;

    if (data_type != -1) {
      /* meta tag data should be stored in the track as a data object */
      if (strncmp (meta_tag, "gnre", 4) == 0) {
	int genre_num = *((short *)buffer) - 1;
      
	/* size includes the \0 */
	size = strlen(genre_table[genre_num]) + 1;

	strcpy (buffer, genre_table[genre_num]);
      }

      dohm_add(tihm, buffer, size, "UTF-8", data_type);
    } else {
      /* meta tag data should be stored in the track object */
      if (strncmp (meta_tag, "trkn", 4) == 0) {
	tihm->track        = big16_2_arch16( ((short *)buffer)[1] );
	tihm->album_tracks = big16_2_arch16( ((short *)buffer)[2] );
      } else if (strncmp (meta_tag, "disk", 4) == 0) {
	tihm->disk_num   = big16_2_arch16( ((short *)buffer)[1] );
	tihm->disk_total = big16_2_arch16( ((short *)buffer)[2] );
      } else if (strncmp (meta_tag, "day", 3) == 0) {
	struct tm release_date;
	
	memset (&release_date, 0, sizeof (struct tm));
	
	sscanf ((char *)buffer, "%04lu-%02lu-%02luT%02lu:%02lu:%02iZ", &release_date.tm_year,
		&release_date.tm_mon, &release_date.tm_mday, &release_date.tm_hour,
		&release_date.tm_min, &release_date.tm_sec);
      
	tihm->year = release_date.tm_year;

	release_date.tm_year -= 1900;
	release_date.tm_mon  -= 1;
	release_date.tm_isdst = -1;
	release_date.tm_zone = "UTC";

	tihm->release_date = mktime (&release_date);
      } else if (strncmp (meta_tag, "tmpo", 4) == 0)
	tihm->bpm = big16_2_arch16 ( ((short *)buffer)[0] );
    }    
  }

  return 0;
}

/* stsz atoms contain a table of sample sizes */
static void mp4_parse_stsz (struct mp4_file *mp4p, unsigned int atom_size) {
  int current_loc = ftell (mp4p->fh);
  unsigned int buffer[3];
  double avg = 0.0;
  double bmax = 0.0;
  double bmin = 10000.0;
  int silent_samples = 0;
  int num_samples = 0;
  unsigned int sample_size;

  int i;

  /* i am unsure how to handle multiple stsz atoms, so only use the first "valid" (size > 20) atom is parsed for data. */
  if (mp4p->bitrate != 0 || atom_size < 0x14)
    return;

  mp3_debug ("mp4.c/parse_stsz: entering...\n");

  fread (buffer, 4, 3, mp4p->fh);

  num_samples = big32_2_arch32 (buffer[2]);

  mp3_debug ("mp4.c/parse_stsz: Atom contains %i samples\n", num_samples);

  for (i = 0 ; i < num_samples ; i++) {
    fread (buffer, 4, 1, mp4p->fh);

    sample_size = big32_2_arch32 (buffer[0]);

    /* Silent samples are 7 bytes in size */
    if (sample_size == 7)
      silent_samples++;
    else
      avg += (double)sample_size;

    bmax = fmax ((double)sample_size, bmax);
    bmin = fmin ((double)sample_size, bmin);
  }
  
  /* because they can throw off the bitrate calculation, silent samples are not used */
  mp4p->bitrate = (unsigned int)BITRATE(avg, num_samples-silent_samples, mp4p->samplerate);
  
  /* I have no idea why this works for apple lossless (or if it works in all cases) */
  if ((mp4p->bitrate - 32) > m4a_bitrates[14]) {
    bmin *= 1000.0/4096.0;
    avg  *= 1000.0/4096.0;
    bmax *= 1000.0/4096.0;

    mp4p->apple_lossless = 1;
    mp4p->bitrate = (unsigned int)BITRATE(avg, num_samples-silent_samples, mp4p->samplerate);
  }

  avg /= (double)(num_samples-silent_samples);
  
  mp3_debug ("mp4.c/parse_stsz: Minimum bitrate is: %f kbps\n", BITRATE(bmin, 1, mp4p->samplerate));
  mp3_debug ("mp4.c/parse_stsz: Agerage bitrate is: %f kbps\n", BITRATE(avg, 1, mp4p->samplerate));
  mp3_debug ("mp4.c/parse_stsz: Maximum bitrate is: %f kbps\n", BITRATE(bmax, 1, mp4p->samplerate));

  fseek (mp4p->fh, current_loc, SEEK_SET);

  mp3_debug ("mp4.c/parse_stsz: complete\n");
}



int mp4_open (char *file_name, struct mp4_file *mp4p) {
  struct qt_atom atom;
  struct stat statinfo;
  char buffer[4];

  mp3_debug ("mp4.c/mp4_open: entering...\n");

  memset (mp4p, 0, sizeof(struct mp4_file));

  if (stat(file_name, &statinfo) < 0)
    return -errno;

  mp4p->file_size = statinfo.st_size;
  mp4p->mtime     = statinfo.st_mtime;
  mp4p->ctime     = statinfo.st_ctime;

  if ((mp4p->fh = fopen (file_name, "r")) == NULL)
    return -errno;

  /* read and check identifier of the file type atom */
  fread (&atom, sizeof(atom), 1, mp4p->fh);
  if (atom.type != string_to_int ("ftyp")) {
    /* not a valid mpeg4 file */
    fclose (mp4p->fh);

    return -1;
  }

  /* read the file type */
  fread (buffer, 1, 4, mp4p->fh);
  if (strncmp (buffer, "mp42", 4) == 0)
    mp4p->faac = 1;
  else if (0 && strncmp (buffer, "M4A ", 4) != 0 &&
	   strncmp (buffer, "M4V ", 4) != 0) {
    fclose (mp4p->fh);

    return -1;
  }

  fseek (mp4p->fh, atom.size - sizeof(atom) - 4, SEEK_CUR);

  mp3_debug ("mp4.c/mp4_open: complete\n");

  return 0;
}

static int mp4_parse_mdhd (struct mp4_file *mp4) {
  struct mdhd mdhd;

  /* MPEG-4 files can contain multiple media headers. Only the first media header with a valid time_scale is used. */
  if (mp4->samplerate != 0)
    return 0;

  fread (&mdhd, sizeof (struct mdhd), 1, mp4->fh);
  fseek (mp4->fh, -sizeof (struct mdhd), SEEK_CUR);

  mp4->samplerate = mdhd.time_scale;
  mp4->duration   = lround (1000.0 * (double)mdhd.duration/(double)mp4->samplerate);

  mp3_debug ("mp4.c/mp4_scan: sample rate = %i, length = %i ms.\n", mp4->samplerate, mp4->duration);

  return 0;
}

int mp4_scan (struct mp4_file *mp4) {
  struct qt_atom atom;
  int i;

  if (mp4 == NULL || mp4->fh == NULL)
    return -EINVAL;

  while (1) {
    if (fread (&atom, sizeof(atom), 1, mp4->fh) != 1) {
      mp3_debug ("mp4.c/mp4_scan: error reading atom header.\n");
      break;
    }

    /* stop parsing on media data atom (audio data follows) */
    if (atom.type == string_to_int ("mdat") || atom.size == 0)
      break;

    if (!is_container(atom.type)) {
      if (atom.type == string_to_int ("meta")) {
        mp4->meta_offset = ftell (mp4->fh);
	mp4->meta_size   = atom.size - sizeof(atom);
      } if (atom.type == string_to_int ("mdhd"))
	mp4_parse_mdhd (mp4);
      else if (atom.type == string_to_int ("stsz"))
	mp4_parse_stsz (mp4, atom.size);
      
      fseek (mp4->fh, atom.size - sizeof(atom), SEEK_CUR);
    }
  }

  if (!mp4->faac && !mp4->apple_lossless) {
    /* find closest match in the m4a bitrate table. only needed for
       itunes/quicktime encoded aac files (not losseless) */
    for (i = 1 ; m4a_bitrates[i] > 0 ; i++) {
      int temp = m4a_bitrates[i-1] - mp4->bitrate;
      int temp2 = m4a_bitrates[i] - mp4->bitrate;

      if (temp <= 0 && (temp + temp2) > 0)
	break;
    }

    mp4->bitrate = m4a_bitrates[i-1];
  }

  if (mp4->bitrate <= 0)
    return -1;

  return 0;
}

void mp4_close (struct mp4_file *mp4) {
  if (mp4 && mp4->fh) {
    fclose (mp4->fh);
    mp4->fh = NULL;
  }
}

int mp4_fill_tihm (char *file_name, tihm_t *tihm) {
  char mp4_type_string[]      = "AAC audio file";
  char m4v_type_string[]      = "MPEG-4 video file";
  char lossless_type_string[] = "Apple Lossless audio file";

  int i, ret;
  struct mp4_file mp4;

  if (file_name == NULL || tihm == NULL)
    return -EINVAL;

  memset (tihm, 0, sizeof (tihm_t));

  ret = mp4_open (file_name, &mp4);
  if (ret != 0)
    return ret;

  mp4_scan (&mp4);
  mp4_parse_meta (&mp4, tihm);
  mp4_close (&mp4);

  tihm->time          = mp4.duration;
  tihm->samplerate    = mp4.samplerate;
  tihm->bitrate       = mp4.bitrate;
  tihm->time          = mp4.duration;
  tihm->size          = mp4.file_size;
  tihm->mod_date      = mp4.mtime;
  tihm->creation_date = mp4.ctime;

  if (strncasecmp (file_name + (strlen(file_name) - 3), "m4v", 3) != 0) {
    tihm->type     = string_to_int ("M4A ");
    tihm->is_video = 0;
    
    if (!mp4.apple_lossless)
      dohm_add (tihm, (u_int8_t *)mp4_type_string, strlen (mp4_type_string), "UTF-8", IPOD_TYPE);
    else
      dohm_add (tihm, (u_int8_t *)lossless_type_string, strlen (lossless_type_string), "UTF-8", IPOD_TYPE);
  } else {
    tihm->type     = string_to_int ("M4V ");
    tihm->is_video = 1;

    dohm_add (tihm, (u_int8_t *)m4v_type_string, strlen (m4v_type_string), "UTF-8", IPOD_TYPE);
  }

  if (tihm->bitrate == 0)
    return -1;

  return 0;
}
