/**
 *   (c) 2003-2004 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.1 tihm.c
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the Lesser GNU Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the Lesser GNU Public License
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

#define TIHM_HEADER_SIZE   0xf4

int db_tihm_search (tree_node_t *entry, u_int32_t tihm_num) {
  int i;

  for ( i = 1 ; i < entry->num_children ; i++ )
    if (((int *)(entry->children[i]->data))[4] == tihm_num)
      return i;

  return -1;
}

int db_tihm_retrieve (ipoddb_t *itunesdb, tree_node_t **entry,
		      tree_node_t **parent, int tihm_num) {
  tree_node_t *dshm_header;
  int entry_num;
  int ret;

  /* find the song list */
  if ((ret = db_dshm_retrieve (itunesdb, &dshm_header, 1)) < 0) {
    db_log (itunesdb, ret, "Could not get song list header\n");
    return ret;
  }

  entry_num = db_tihm_search (dshm_header, tihm_num);

  if (entry_num < 0) return entry_num;

  if (entry) *entry = dshm_header->children[entry_num];
  if (parent)*parent= dshm_header;

  return entry_num;
}

/* fills a tree_node with the data from a tihm_t structure */
int tihm_db_fill (tree_node_t *tihm_header, tihm_t *tihm) {
  struct db_tihm *tihm_data;

  tihm_data = (struct db_tihm *)tihm_header->data;
  tihm_data->num_dohm    = tihm->num_dohm;
  tihm_data->identifier  = tihm->num;
  tihm_data->type        = 0x001;

  tihm_data->flags      |= 0x100;
  tihm_data->flags      |= ((tihm->stars % 6) * 0x14) << 24;
  /* lowest flag bit(byte?) is vrb */
  tihm_data->flags      |= tihm->vbr;

  tihm_data->creation_date = tihm->creation_date;
  tihm_data->modification_date = tihm->mod_date;
  tihm_data->last_played_date = time(NULL);

  tihm_data->num_played[0] = 0;
  tihm_data->num_played[1] = 0;

  tihm_data->file_size   = tihm->size;
  tihm_data->duration    = tihm->time;

  tihm_data->order       = tihm->track;
  tihm_data->album_tracks= tihm->album_tracks;

  tihm_data->disk_num    = tihm->disk_num;
  tihm_data->disk_total  = tihm->disk_total;

  tihm_data->sample_rate = tihm->samplerate << 16;
  tihm_data->bit_rate    = tihm->bitrate;
  tihm_data->unk11       = tihm->bpm << 16;

  tihm_data->year        = tihm->year;

  tihm_data->volume_adjustment = tihm->volume_adjustment;
  tihm_data->start_time  = tihm->start_time;
  tihm_data->stop_time   = tihm->stop_time;

  tihm_data->has_artwork = 0xffffffff;
  /* it may be useful to set other values in the tihm structure but many
     have still not been deciphered */

  return 0;
}

int tihm_fill_from_file (tihm_t *tihm, char *path, u_int8_t *ipod_path, size_t path_len,
			 int stars, int tihm_num, int ipod_use_unicode_hack) {
  if (tihm == NULL)
    return -1;

  memset (tihm, 0, sizeof (tihm_t));

  if (strncasecmp (path + (strlen(path) - 3), "mp3", 3) == 0) {
    if (mp3_fill_tihm (path, tihm) < 0) {
      tihm_free (tihm); /* structure may have been partially filled before error */
      
      return -1;
    }
  } else if ( (strncasecmp (path + (strlen(path) - 3), "m4a", 3) == 0)  ||
	      (strncasecmp (path + (strlen(path) - 3), "m4p", 3) == 0)  ||
	      (strncasecmp (path + (strlen(path) - 3), "aac", 3) == 0) ) {
    if (aac_fill_tihm (path, tihm) < 0) {
      tihm_free (tihm); /* structure may have been partially filled before error */

      return -1;
    }
  } else
    return -1;
  
  dohm_add_path (tihm, ipod_path, path_len, "UTF-8", IPOD_PATH, ipod_use_unicode_hack);

  tihm->num = tihm_num;
  tihm->stars = stars;
  
  return 0;
}

/*
 db_tihm_create:
 
 Creates a database entry from a structure describing the song and stores the
 data into the location pointed to by entry.
*/
int db_tihm_create (tree_node_t **entry, tihm_t *tihm) {
  tree_node_t *dohm;
  int i, ret;

  if ((ret = db_node_allocate (entry, TIHM, TIHM_HEADER_SIZE, TIHM_HEADER_SIZE)) < 0)
    return ret;
  
  tihm_db_fill (*entry, tihm);
  
  for (i = 0 ; i < tihm->num_dohm ; i++) {
    if (db_dohm_create (&dohm, tihm->dohms[i], 16) < 0)
      return -1;

    db_attach (*entry, dohm);
  }
  
  return 0;
}

static int tihm_fill_from_database_entry (tihm_t *tihm, tree_node_t *entry) {
  struct db_tihm *dbtihm = (struct db_tihm *)entry->data;
  
  tihm->num_dohm  = dbtihm->num_dohm;
  tihm->num       = dbtihm->identifier;
  tihm->size      = dbtihm->file_size;
  tihm->time      = dbtihm->duration;
  tihm->samplerate= dbtihm->sample_rate >> 16;
  tihm->bitrate   = dbtihm->bit_rate;
  tihm->times_played = dbtihm->num_played[0];

  tihm->stars     = (dbtihm->flags >> 24) / 0x14;

  tihm->type      = dbtihm->type;
  tihm->album_tracks= dbtihm->album_tracks;

  tihm->disk_num  = dbtihm->disk_num;
  tihm->disk_total= dbtihm->disk_total;

  tihm->track     = dbtihm->order;
  tihm->volume_adjustment = dbtihm->volume_adjustment;
  tihm->start_time= dbtihm->start_time;
  tihm->stop_time = dbtihm->stop_time;
  tihm->bpm       = dbtihm->unk11 >> 16;

  tihm->year        = dbtihm->year;

  tihm->mod_date  = dbtihm->modification_date;
  tihm->played_date = dbtihm->last_played_date;
  tihm->creation_date = dbtihm->creation_date;

  tihm->dohms     = db_dohm_fill (entry);

  return 0;
}


tihm_t *db_tihm_fill (tree_node_t *entry) {
  tihm_t *tihm;
  int ret;

  tihm = (tihm_t *) calloc (1, sizeof(tihm_t));
  if (tihm == NULL) {
    perror ("db_tihm_fill|calloc");

    return NULL;
  }

  if ((ret = tihm_fill_from_database_entry (tihm, entry)) != 0) {
    free (tihm);

    return NULL;
  }

  return tihm;
}

/**
   db_song_modify:

   Updates the song database entry tihm_num.

   Arguments:
    ipoddb_t *itunesdb - opened itunesdb
    int         tihm_num - tihm reference
    tihm_t     *tihm     - data to change

   Returns:
    -1 on error
     0 on success
**/
int db_song_modify (ipoddb_t *itunesdb, int tihm_num, tihm_t *tihm) {
  tree_node_t *tihm_header;

  if (db_tihm_retrieve (itunesdb, &tihm_header, NULL, tihm_num) < 0)
    return -1;

  tihm_db_fill (tihm_header, tihm);

  /* The database entry's size will not change so it is unnecessary to
     update size fields for the parents. */

  return 0;
}

void tihm_free (tihm_t *tihm) {
  if (tihm == NULL)
    return;

  dohm_free (tihm->dohms, tihm->num_dohm);
}
