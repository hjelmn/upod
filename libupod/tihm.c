/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a tihm.c
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

#define TIHM_HEADER_SIZE   0x9c

int db_tihm_search (tree_node_t *entry, u_int32_t tihm_num) {
  int i;

  for ( i = 1 ; i < entry->num_children ; i++ )
    if (((int *)(entry->children[i]->data))[4] == tihm_num)
      return i;

  return -1;
}

int db_tihm_retrieve (itunesdb_t *itunesdb, tree_node_t **entry,
		      tree_node_t **parent, int tihm_num) {
  tree_node_t *root, *dshm_header;
  struct db_dshm *dshm_data;
  int i, entry_num;

  if (itunesdb->tree_root == NULL) return -1;
  root = itunesdb->tree_root;

  /* find the song list */
  for (i = 0 ; i < root->num_children ; i++) {
    dshm_header = (tree_node_t *)root->children[i];
    dshm_data = (struct db_dshm *) dshm_header->data;
    
    if (dshm_data->dshm == DSHM && dshm_data->type == 0x1)
      break;
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

  if (tihm_header->data)
    free (tihm_header->data);

  tihm_header->size = TIHM_HEADER_SIZE;
  tihm_header->data = calloc (1, TIHM_HEADER_SIZE);
  memset (tihm_header->data, 0, TIHM_HEADER_SIZE);
  
  tihm_data = (struct db_tihm *)tihm_header->data;
  tihm_data->tihm        = TIHM;
  tihm_data->header_size = TIHM_HEADER_SIZE;
  tihm_data->record_size = TIHM_HEADER_SIZE;
  tihm_data->num_dohm    = tihm->num_dohm;
  tihm_data->identifier  = tihm->num;
  tihm_data->type        = 0x001;

  tihm_data->flags      |= 0x100;
  tihm_data->flags      |= ((tihm->stars % 6) * 0x14) << 24;

  tihm_data->creation_date = tihm->creation_date;
  tihm_data->modification_date = tihm->mod_date;
  tihm_data->last_played_date = time(NULL);

  tihm_data->num_played[0] = 0;
  tihm_data->num_played[1] = 0;

  tihm_data->file_size   = tihm->size;
  tihm_data->duration    = tihm->time;
  tihm_data->order       = tihm->track;
  tihm_data->sample_rate = tihm->samplerate << 16;
  tihm_data->bit_rate    = tihm->bitrate;

  tihm_data->volume_adjustment = tihm->volume_adjustment;
  tihm_data->start_time  = tihm->start_time;
  tihm_data->stop_time   = tihm->stop_time;

  /* there may be other values wich should be set but i dont know
     which */
}

int db_tihm_create (tree_node_t *entry, char *path, char *ipod_path, int path_len, int stars) {
  tree_node_t *dohm;
  dohm_t *dohm_data;
  tihm_t tihm;
  int tihm_num = ((int *)(entry->parent->children[entry->parent->num_children - 1]->data))[4] + 1;
  int i;

  memset (entry, 0, sizeof (tree_node_t));
  if (strcasecmp (path + (strlen(path) - 3), "mp3") == 0) {
    if (mp3_fill_tihm (path, &tihm) < 0) {
      fprintf (stderr, "Invalid MP3 file: %s\n", path);
      return -1;
    }
  } else if ( (strcasecmp (path + (strlen(path) - 3), "m4a") == 0)  ||
	      (strcasecmp (path + (strlen(path) - 3), "aac") == 0) ) {
    if (aac_fill_tihm (path, &tihm) < 0) {
      fprintf (stderr, "Invalid AAC file: %s\n", path);
      return -1;
    }
  } else {
    fprintf (stderr, "Unrecognized file format (Using extension)\n");
    return -1;
  }

  tihm.num = tihm_num;
  tihm.stars = stars;

  dohm_data = dohm_create (&tihm);
  dohm_data->type = IPOD_PATH;
  unicode_check_and_copy ((char **)&(dohm_data->data), &dohm_data->size, ipod_path, path_len);

  tihm_db_fill (entry, &tihm);

  for (i = 0 ; i < tihm.num_dohm ; i++) {
    dohm = (tree_node_t *) calloc (1, sizeof(tree_node_t));

    if (dohm == NULL) {
      perror ("db_create_tihm|calloc");
      return -1;
    }

    if (db_dohm_create (dohm, tihm.dohms[i]) < 0)
      return -1;

    db_attach (entry, dohm);
  }

  return tihm_num;
}

tihm_t *db_tihm_fill (tree_node_t *entry) {
  int *iptr = (int *)entry->data;
  struct db_tihm *dbtihm = (struct db_tihm *)entry->data;
  tihm_t *tihm;

  tihm = (tihm_t *) calloc (1, sizeof(tihm_t));
  
  tihm->num_dohm  = dbtihm->num_dohm;
  tihm->num       = dbtihm->identifier;
  tihm->size      = dbtihm->file_size;
  tihm->time      = dbtihm->duration;
  tihm->samplerate= dbtihm->sample_rate >> 16;
  tihm->bitrate   = dbtihm->bit_rate;
  tihm->times_played = dbtihm->num_played[0];

  tihm->stars     = (dbtihm->flags >> 24) / 0x14;
  tihm->type      = dbtihm->type;
  tihm->track     = dbtihm->order;
  tihm->volume_adjustment = dbtihm->volume_adjustment;
  tihm->start_time= dbtihm->start_time;
  tihm->stop_time = dbtihm->stop_time;

  tihm->mod_date  = dbtihm->modification_date;
  tihm->played_date = dbtihm->last_played_date;
  tihm->creation_date = dbtihm->creation_date;

  tihm->dohms     = db_dohm_fill (entry);
  return tihm;
}

/**
   db_song_modify:

    Modifies the data of a song entry. Be sure that all the values you
   want to change are correct before calling this function; It will screw
   up that entry if you don't.

   Arguments:
    itunesdb_t *itunesdb - opened itunesdb
    int         tihm_num - tihm reference
    tihm_t     *tihm     - data to change

   Returns:
    -1 on error
     0 on success
**/
int db_song_modify (itunesdb_t *itunesdb, int tihm_num, tihm_t *tihm) {
  tree_node_t *tihm_header;

  if (db_tihm_retrieve (itunesdb, &tihm_header, NULL, tihm_num) < 0)
    return -1;

  tihm_db_fill (tihm_header, tihm);

  return 0;
}

void tihm_free (tihm_t *tihm) {
  if (tihm == NULL) return;

  dohm_free (tihm->dohms, tihm->num_dohm);
}
