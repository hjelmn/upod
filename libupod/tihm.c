/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.1 tihm.c
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

#include "itunesdbi.h"

int db_tihm_search (tree_node_t *entry, u_int32_t tihm_num) {
  int i;

  for ( i = 1 ; i < entry->num_children ; i++ )
    if (((int *)(entry->children[i]->data))[4] == tihm_num)
      return i;

  return -1;
}

int db_tihm_retrieve (ipoddb_t *itunesdb, tree_node_t **entry, tree_node_t **parent,
		      int tihm_num) {
  tree_node_t *dshm_header;
  int entry_num;
  int ret;

  if (itunesdb == NULL || itunesdb->type != 0)
    return -EINVAL;

  /* find the song list */
  if ((ret = db_dshm_retrieve (itunesdb, &dshm_header, 1)) < 0) {
    db_log (itunesdb, ret, "db_tihm_retrieve: Could not get song list header\n");
    return ret;
  }

  entry_num = db_tihm_search (dshm_header, tihm_num);

  if (entry_num < 0) return entry_num;

  if (entry) *entry = dshm_header->children[entry_num];
  if (parent)*parent= dshm_header;

  return entry_num;
}

static int db_sort (tree_node_t *dshm_header, int sort_by, u_int32_t *list, u_int32_t *tmp,
		    int list_length) {
  int i, j, k;

  tree_node_t *dohm_header1, *dohm_header2;

  if (list_length < 2)
    return 0;
  
  db_sort (dshm_header, sort_by, list, tmp, list_length/2);
  db_sort (dshm_header, sort_by, &list[list_length/2], tmp, list_length - list_length/2);

  db_dohm_retrieve (dshm_header->children[1 + list[0]], &dohm_header1, sort_by);
  db_dohm_retrieve (dshm_header->children[1 + list[list_length/2]], &dohm_header2, sort_by);

  /* merge */
  for (i = 0, j = list_length/2, k = 0 ; (i < list_length/2) && (j < list_length) ; k++) {
    if (db_dohm_compare (dohm_header1, dohm_header2) <= 0) {
      tmp[k] = list[i++];

      if (i != list_length/2)
	db_dohm_retrieve (dshm_header->children[1 + list[i]], &dohm_header1, sort_by);
    } else {
      tmp[k] = list[j++];

      if (j != list_length)
	db_dohm_retrieve (dshm_header->children[1 + list[j]], &dohm_header2, sort_by);
    }
  }

  while (i < list_length/2)
    tmp[k++] = list[i++];
  while (j < list_length)
    tmp[k++] = list[j++];

  memcpy (list, tmp, 4 * list_length);

  return 0;
}

int db_tihm_get_sorted_indices (ipoddb_t *itunesdb, int sort_by, u_int32_t **indices, u_int32_t *num_indices) {
  tree_node_t *dshm_header;
  tree_node_t *tlhm_header;

  db_tlhm_t *tlhm_data;
  u_int32_t *tmp;

  int i, ret;

  if (itunesdb == NULL || indices == NULL || num_indices == NULL)
    return -EINVAL;

  /* find the song list */
  if ((ret = db_dshm_retrieve (itunesdb, &dshm_header, 1)) < 0) {
    db_log (itunesdb, ret, "Could not get song list header\n");
    return ret;
  }

  tlhm_header = dshm_header->children[0];
  tlhm_data = (db_tlhm_t *)tlhm_header->data;

  *indices = (u_int32_t *) calloc (tlhm_data->list_entries, 4);

  if (*indices == NULL) {
    perror ("db_tihm_get_sorted_indices|calloc");
    
    exit (EXIT_FAILURE);
  }

  for (i = 0 ; i < tlhm_data->list_entries ; i++)
    (*indices)[i] = i;

  tmp = (u_int32_t *) calloc (tlhm_data->list_entries, 4);

  if (tmp == NULL) {
    perror ("db_tihm_get_sorted_indices|calloc");
    
    exit (EXIT_FAILURE);
  }

  db_sort (dshm_header, sort_by, *indices, tmp, tlhm_data->list_entries);

  *num_indices = tlhm_data->list_entries;

  free (tmp);

  return 0;
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

  tihm_data->creation_date     = tihm->creation_date;
  tihm_data->modification_date = tihm->mod_date;
  tihm_data->last_played_date  = 0;

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
  tihm_data->unk1        = tihm->bpm << 16;

  tihm_data->year        = tihm->year;

  tihm_data->volume_adjustment = tihm->volume_adjustment;
  tihm_data->start_time  = tihm->start_time;
  tihm_data->stop_time   = tihm->stop_time;

  if (tihm->has_artwork) {
    tihm_data->has_artwork = 0xffff0001;
    tihm_data->iihm_id  = tihm->artwork_id;
  } else
    tihm_data->has_artwork = 0xffffffff;

  /* it may be useful to set other values in the tihm structure but many
     have still not been deciphered */

  return 0;
}

int tihm_fill_from_file (tihm_t *tihm, char *path, char *ipod_path, int stars, int tihm_num) {
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
  
  dohm_add (tihm, ipod_path, strlen(ipod_path), "UTF-8", IPOD_PATH);

  tihm->num = tihm_num;
  tihm->stars = stars;
  
  return 0;
}

/*
 db_tihm_create:
 
 Creates a database entry from a structure describing the song and stores the
 data into the location pointed to by entry.
*/
int db_tihm_create (tree_node_t **entry, tihm_t *tihm, int flags) {
  tree_node_t *dohm;
  int i, ret;

  if ((ret = db_node_allocate (entry, TIHM, TIHM_CELL_SIZE, TIHM_CELL_SIZE)) < 0)
    return ret;
  
  tihm_db_fill (*entry, tihm);
  
  for (i = 0 ; i < tihm->num_dohm ; i++) {
    if (db_dohm_create (&dohm, tihm->dohms[i], 16, flags) < 0)
      return -1;

    db_attach (*entry, dohm);
  }
  
  return 0;
}

int db_tihm_fill (tree_node_t *tihm_header, tihm_t *tihm) {
  struct db_tihm *tihm_data;
  
  if (tihm_header == NULL || tihm == NULL)
    return -EINVAL;

  tihm_data = (struct db_tihm *)tihm_header->data;

  if (tihm_data->tihm != TIHM)
    return -EINVAL;

  tihm->num_dohm  = tihm_data->num_dohm;
  tihm->num       = tihm_data->identifier;
  tihm->size      = tihm_data->file_size;
  tihm->time      = tihm_data->duration;
  tihm->samplerate= tihm_data->sample_rate >> 16;
  tihm->bitrate   = tihm_data->bit_rate;
  tihm->times_played = tihm_data->num_played[0];

  tihm->stars     = (tihm_data->flags >> 24) / 0x14;

  tihm->type      = tihm_data->type;
  tihm->album_tracks= tihm_data->album_tracks;

  tihm->disk_num  = tihm_data->disk_num;
  tihm->disk_total= tihm_data->disk_total;

  tihm->track     = tihm_data->order;
  tihm->volume_adjustment = tihm_data->volume_adjustment;
  tihm->start_time= tihm_data->start_time;
  tihm->stop_time = tihm_data->stop_time;
  tihm->bpm       = tihm_data->unk1 >> 16;

  tihm->year        = tihm_data->year;

  tihm->mod_date      = tihm_data->modification_date;
  tihm->played_date   = tihm_data->last_played_date;
  tihm->creation_date = tihm_data->creation_date;

  tihm->has_artwork = (tihm_data->has_artwork != 0xffffffff) ? 1 : 0;
  tihm->artwork_id  = tihm_data->iihm_id;
  
  db_dohm_fill (tihm_header, &(tihm->dohms));

  return 0;
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

  if (tihm->image_data)
    free (tihm->image_data);

  dohm_free (tihm->dohms, tihm->num_dohm);
}
