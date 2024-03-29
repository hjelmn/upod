/**
 *   (c) 2002-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.4.0 playlist.c
 *
 *   Functions for managing playlists on the iPod.
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

/*
  db_playlist_retrieve:

  Internal function that saves a pointer to the database's plhm and playlist
  dshm to the pointers that are passed in.

  Arguments:
   ipoddb_t *itunesdb         - opened itunesdb
   db_plhm_t **plhm_data - pointer to location to store plhm data
   tree_node  **dshm_header   - pointer to location of desired parent pointer.
   int playlist               - playlist to return
   tree_node  **pyhm_header   - pointer to location to store playlist header

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_retrieve (ipoddb_t *itunesdb, db_plhm_t **plhm_data, tree_node_t **dshm_header,
			  int playlist, int data_section, tree_node_t **pyhm_header) {
  tree_node_t *temp, *plhm_header;
  db_plhm_t *plhm_data_loc;
  int ret;

  if (itunesdb == NULL || itunesdb->type != 0 || (playlist < 0 && pyhm_header == NULL)) {
    db_log (itunesdb, -EINVAL, "Invalid argument\n");
    return -EINVAL;
  }

  if ((ret = db_dshm_retrieve (itunesdb, &temp, data_section)) < 0) {
    db_log (itunesdb, ret, "Unable to retrieve a playlist data storage entry (MAJOR ERROR).\n");
    return ret;
  }

  plhm_header = temp->children[0];

  plhm_data_loc = (db_plhm_t *)plhm_header->data;

  /* Store the pointers if storage was passed in */
  if (plhm_data != NULL)
    *plhm_data = plhm_data_loc;

  if (dshm_header != NULL)
    *dshm_header = temp;

  if (pyhm_header != NULL) {
    if (playlist > (plhm_data_loc->list_entries - 1)) {
      db_log (itunesdb, 0, "Requested playlist (%i) does not exist. There are %i playlists.\n", playlist,
	      plhm_data_loc->list_entries);
      return -EINVAL;
    }

    *pyhm_header = temp->children[playlist + 1];
  }

  return 0;
}

/**
  db_playlist_number:

  Returns the number of playlists in the itunesdb.

  Arguments:
   ipoddb_t *itunesdb - open itunesdb
   int data_section   - typically 2 for music or 3 for video

  Returns:
   < 1 on error
   number of playlists on success
**/
int db_playlist_number (ipoddb_t *itunesdb, int data_section) {
  db_plhm_t *plhm_data;
  int ret;

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, NULL, 0, data_section, NULL)) < 0)
    return ret;

  return plhm_data->list_entries;
}

/**
  db_playlists_list:

  Returns a linked list containing the numbers and names of the playlists in
  the itunesdb.

  Arguments:
   ipoddb_t *itunesdb - opened itunesdb

  Returns:
   NULL on error
   pointer to head of linked list on success
**/
int db_playlist_list (ipoddb_t *itunesdb, db_list_t **head, int data_section) {
  db_plhm_t *plhm_data;
  char *temp;

  int i, ret;

  if (head == NULL)
    return -EINVAL;
  else
    *head = NULL;

  db_log (itunesdb, 0, "db_playlist_list: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, NULL, 0, data_section, NULL)) < 0)
    return ret;

  for ( i = plhm_data->list_entries - 1 ; i >= 0 ; i--) {
    struct pyhm *pyhm;

    if ((ret = db_playlist_get_name (itunesdb, i, data_section, &temp)) < 0)
      continue;

    pyhm     = calloc (1, sizeof (struct pyhm));
    pyhm->num  = i;
    pyhm->name = temp;
    pyhm->name_len = strlen ((char *)temp);

    *head = db_list_prepend (*head, pyhm);
  }

  db_log (itunesdb, 0, "db_playlist_list: complete\n");

  return 0;
}

/**
  db_playlist_list_free:

  Frees the memory used by a db_list_t containing playlist information.

  Arguments:
   db_list_t **head - pointer to head of allocated playlist list

  Returns:
   nothing, void function
**/
void db_playlist_list_free (db_list_t **head) {
  db_list_t *tmp;
  if (head == NULL)
    return;

  for (tmp = db_list_first (*head) ; tmp ; tmp = db_list_next (tmp)) {
    struct pyhm *pyhm = (struct pyhm *)tmp->data;

    free (pyhm->name);
    free (tmp->data);
  }

  db_list_free (*head);

  *head = NULL;
}

/**
  db_playlist_list_songs:

  Stores an integer list of all the references contained in a playlist.

  Argument:
   ipoddb_t *itunesdb - opened itunesdb
   int         playlist - playlist index (0 is master)
   int       **list     - pointer to where the list should go

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_song_list (ipoddb_t *itunesdb, int playlist, db_list_t **head) {
  return db_playlist_track_list (itunesdb, playlist, head, 2);
}

int db_playlist_video_list (ipoddb_t *itunesdb, int playlist, db_list_t **head) {
  return db_playlist_track_list (itunesdb, playlist, head, 3);
}

int db_playlist_track_list (ipoddb_t *itunesdb, int playlist, db_list_t **head, int data_section) {
  tree_node_t *pyhm_header;

  struct db_pyhm *pyhm_data;
  struct db_pihm *pihm_data;

  int i;
  int num_entries = 0;

  int ret;

  if (head == NULL)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_song_list: entering...\n");

  *head = NULL;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0) {
    db_log (itunesdb, ret, "Could not retrive playlist header.\n");
    return ret;
  }

  pyhm_data = (struct db_pyhm *) pyhm_header->data;

  if (pyhm_data->num_pihm > 0) {
    num_entries = (pyhm_header->num_children - 2)/2;
    
    for (i = pyhm_header->num_children - 1 ; i >= pyhm_data->num_dohm ; i--) {
      pihm_data = (struct db_pihm *)pyhm_header->children[i]->data;

      if (pihm_data->pihm != PIHM)
	continue;

      (*head) = db_list_prepend (*head, (void *)pihm_data->reference);
    }
  }

  db_log (itunesdb, 0, "db_playlist_song_list: complete\n");

  return num_entries;
}

void db_playlist_song_list_free (db_list_t **head) {
  if (head == NULL)
    return;

  db_list_free (*head);
  head = NULL;
}

/**
  db_playlist_create:

   Creates a new playlist on the ipod with name.

  Arguments:
   ipoddb_t *itunesdb - Opened itunesdb
   char       *name     - Name of new playlist
   int         name_len - length of buffer name

  Returns:
   < 0         on error
   playlist id on success
**/
static int db_playlist_create_loc (ipoddb_t *itunesdb, char *name, int data_section,
				   char podcast_flag) {
  tree_node_t *new_pyhm, *new_dohm, *dshm_header;
  db_plhm_t *plhm_data;

  int ret;

  dohm_t dohm;

  if (itunesdb == NULL || name == NULL)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_create: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, &dshm_header, 0, data_section, NULL)) < 0)
    return ret;

  dohm.data = name;

  if ((ret = db_pyhm_create (&new_pyhm, plhm_data->list_entries == 0)) < 0)
    return ret;

  db_pyhm_set_podcast (new_pyhm, podcast_flag);

  /* Give the playlist a random ID */
  db_pyhm_set_id (new_pyhm, rand ());

  if ((ret = db_dohm_itunes_create (&new_dohm)) < 0)
    return ret;

  db_dohm_itunes_show (new_dohm, SHOW_PLAYING    , 0x12);
  db_dohm_itunes_show (new_dohm, SHOW_TITLE      , 0x9d);
  db_dohm_itunes_show (new_dohm, SHOW_TIME       , 0x35);

  db_dohm_itunes_show (new_dohm, SHOW_DISK_NUMBER, 0x2e);
  db_dohm_itunes_show (new_dohm, SHOW_TRACK_NUM  , 0x3c);
  db_dohm_itunes_show (new_dohm, SHOW_ALBUM      , 0x9d);
  db_dohm_itunes_show (new_dohm, SHOW_ARTIST     , 0x9d);
  db_dohm_itunes_show (new_dohm, SHOW_YEAR       , 0x25);
  db_dohm_itunes_show (new_dohm, SHOW_BITRATE    , 0x5d);
  db_dohm_itunes_show (new_dohm, SHOW_SAMPLERATE , 0x4e);

  db_pyhm_dohm_attach (new_pyhm, new_dohm);

  /* create the title entry for the new playlist */
  dohm.type = IPOD_TITLE;
  if ((ret = db_dohm_create (&new_dohm, dohm, 16, 0)) < 0)
    return ret;

  db_pyhm_dohm_attach (new_pyhm, new_dohm);

  /* this MUST be done last to avoid adding the sizes it's children twice */
  db_attach (dshm_header, new_pyhm);

  db_log (itunesdb, 0, "db_playlist_create: complete\n");

  return plhm_data->list_entries++;
}

int db_playlist_create (ipoddb_t *itunesdb, char *name, int data_section) {
  return db_playlist_create_loc (itunesdb, name, data_section, 0);
}

int db_playlist_create_podcast (ipoddb_t *itunesdb, char *name, int data_section) {
  return db_playlist_create_loc (itunesdb, name, data_section, 1);
}

/**
  db_playlist_rename:

   Rename a playlist in the itunesdb.

  Arguments:
   ipoddb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is master)
   char       *name     - New playlist name
   int         name_len - Lenth of name
**/
int db_playlist_rename (ipoddb_t *itunesdb, int playlist, int data_section, u_int8_t *name) {
  tree_node_t *dohm_header, *pyhm_header, *new_dohm_header;

  struct db_dohm *dohm_data;
  struct db_pyhm *pyhm_data;

  int i, ret;

  dohm_t dohm;

  if (itunesdb == NULL || name == NULL)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_rename: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  pyhm_data = (struct db_pyhm *)pyhm_header->data;

  /* Find the playlist name (should be a title dohm) */
  for (i = 0 ; i < pyhm_data->num_dohm ; i++) {
    dohm_header = pyhm_header->children[i];
    dohm_data   = (struct db_dohm *)dohm_header->data;

    if (dohm_data->type == IPOD_TITLE)
      break;
  }

  if (i == pyhm_data->num_dohm)
    return -1;

  dohm.data = name;
  dohm.type = IPOD_TITLE;

  if ((ret = db_dohm_create (&new_dohm_header, dohm, 16, 0)) < 0)
    return ret;

  /* no need to worry about modifying the dohm count since it won't change */
  db_detach (pyhm_header, i, &dohm_header);
  db_free_tree (dohm_header);

  db_attach_at (pyhm_header, new_dohm_header, i);

  db_log (itunesdb, 0, "db_playlist_rename: complete\n");

  return 0;
}

/**
  db_playlist_delete:
    
    Delete a playlist from the itunesdb.

  Arguments:
   ipoddb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is master)

  Returns:
   < 0 on error or when attempting to remove the master playlist
     0 on success
**/
int db_playlist_delete (ipoddb_t *itunesdb, int playlist, int data_section) {
  tree_node_t *dshm_header, *pyhm_header;
  db_plhm_t *plhm_data;

  int ret;

  /* Can't delete the main playlist */
  if (itunesdb == NULL || playlist == 0)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_delete: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, &dshm_header, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  plhm_data->list_entries--;

  db_detach (dshm_header, playlist + 1, &pyhm_header);
  db_free_tree (pyhm_header);

  db_log (itunesdb, 0, "db_playlist_delete: complete\n");

  return 0;
}

/**
  db_playlist_tihm_add:

    Adds a reference to song tihm_num to playlist.

  Arguments:
   ipoddb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is the master playlist)
   int         tihm_num - Song reference to remove

  Returns:
   < 0 on error
     0 on success
**/
int db_playlist_tihm_add (ipoddb_t *itunesdb, int playlist, int data_section, int tihm_num) {
  tree_node_t *pyhm_header, *pihm_header;
  struct db_pyhm *pyhm_data;
  int order;
  int entry_num, ret;
  char *name;

  if (itunesdb == NULL)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_tihm_add: entering...\n");

  /* make sure the tihm exists in the database before continuing */
  if ((ret = db_tihm_retrieve (itunesdb, NULL, NULL, tihm_num)) < 0)
    return ret;
    
  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  /* check if the reference already exists in this playlist */
  entry_num = db_pihm_search (pyhm_header, tihm_num);

  if (pyhm_data->num_pihm != 0) {
    struct db_pihm *pihm_data = (struct db_pihm *)pyhm_header->children[pyhm_header->num_children - 1];

    order = pihm_data->order + 1;
  } else
    /* 100000 after the last song entry should be a safe number to start at */
    order = itunesdb->last_entry + 100000;

  if (entry_num == -1) {
    db_pihm_create (&pihm_header, tihm_num, order, 0);
    db_attach (pyhm_header, pihm_header);
    
    pyhm_data->num_pihm += 1;
  }

  db_log (itunesdb, 0, "db_playlist_tihm_add: complete\n");

  return 0;
}

/**
  db_playlist_tihm_remove:

    Removes the reference to song tihm_num from playlist.

  Arguments:
   ipoddb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is the master playlist)
   int         tihm_num - Song reference to remove

  Returns:
   < 0 on error
     0 on success
**/
int db_playlist_tihm_remove (ipoddb_t *itunesdb, int playlist, int data_section, int tihm_num) {
  tree_node_t *pyhm_header, *pihm_header, *dohm_header;
  struct db_pyhm *pyhm_data;

  int entry_num, ret;

  if (itunesdb == NULL)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_tihm_remove: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  /* search and destroy */
  entry_num = db_pihm_search (pyhm_header, tihm_num);

  if (entry_num > -1) {
    db_detach (pyhm_header, entry_num, &pihm_header);
    db_detach (pyhm_header, entry_num, &dohm_header);
    
    db_free_tree (pihm_header);
    db_free_tree (dohm_header);

    pyhm_data->num_pihm -= 1;
  }

  db_log (itunesdb, 0, "db_playlist_tihm_remove: complete\n");

  return 0;
}

/*
  db_playlist_clear:

    Clears all references from playlist.

  Arguments:
   ipoddb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is the master playlist)

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_clear (ipoddb_t *itunesdb, int playlist, int data_section) {
  tree_node_t *pyhm_header, *pihm_header, *dohm_header;

  struct db_pyhm *pyhm_data;

  int ret;
  
  if (itunesdb == NULL)
    return -EINVAL;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  while (pyhm_data->num_pihm) {
    struct db_pihm *pihm_data = (struct db_pihm *)pyhm_header->children[pyhm_header->num_children - 1];

    if (pihm_data->pihm == DOHM) {
      db_detach (pyhm_header, pyhm_header->num_children, &dohm_header);
      db_free_tree (dohm_header);
    }

    db_detach (pyhm_header, pyhm_header->num_children, &pihm_header);

    db_free_tree (pihm_header);

    pyhm_data->num_pihm--;
  }

  return 0;
}

/*
  db_playlist_fill:

    Fills a playlist with references to every song.

  Arguments:
   ipoddb_t *itunesdb - Opened itunesdb
   int       playlist - Playlist index (0 is the master playlist)

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_fill (ipoddb_t *itunesdb, int playlist, int data_section) {
  tree_node_t *track_dshm, *tlhm_header;

  db_tlhm_t *tlhm_data;
  struct db_tihm *tihm_data;

  int i, ret;
  
  if ((ret = db_playlist_clear (itunesdb, playlist, data_section)) < 0)
    return ret;

  if ((ret = db_dshm_retrieve (itunesdb, &track_dshm, 1)) < 0)
    return -EINVAL;

  tlhm_header= track_dshm->children[0];
  tlhm_data  = (db_tlhm_t *)tlhm_header->data;

  for (i = 0 ; i < tlhm_data->list_entries ; i++)
    db_playlist_tihm_add (itunesdb, playlist, tihm_data->identifier, data_section);

  return 0;
}

/* remove a track reference from every playlist */
static int db_playlist_remove_all_ds (ipoddb_t *itunesdb, int tihm_num, int data_section) {
  int i, total;

  if (itunesdb == NULL)
    return -EINVAL;

  if ((total = db_playlist_number (itunesdb, data_section)) < 0)
    return total;
  
  for (i = 0 ; i < total ; i++)
    db_playlist_tihm_remove (itunesdb, i, tihm_num, data_section);

  return 0;
}

int db_playlist_remove_all (ipoddb_t *itunesdb, int tihm_num) {
  return db_playlist_remove_all_ds (itunesdb, tihm_num, (db_isvideo (itunesdb, tihm_num)) ? 3 : 2);
}

int db_playlist_column_show (ipoddb_t *itunesdb, int playlist, int data_section, int column, u_int16_t width) {
  tree_node_t *pyhm_header;

  int ret;

  if (itunesdb == NULL)
    return -EINVAL;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  return db_dohm_itunes_show (pyhm_header->children[0], column, width);
}

int db_playlist_column_hide (ipoddb_t *itunesdb, int playlist, int data_section, int column) {
  tree_node_t *pyhm_header;

  int ret;

  if (itunesdb == NULL)
    return -EINVAL;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  return db_dohm_itunes_hide (pyhm_header, column);
}

/*
  first position is position 0
*/
int db_playlist_column_move (ipoddb_t *itunesdb, int playlist, int data_section, int cola, int pos) {
  tree_node_t *pyhm_header, *wierd_dohm_header = NULL;
  int num_shown, ret, j, finish;

  struct db_wierd_dohm *wierd_dohm_data;
  struct db_pyhm *pyhm_data;
  struct db_dohm *dohm_data;

  if (cola < 0 || pos < 0 || cola == pos || itunesdb == NULL)
    return -EINVAL;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  pyhm_data = (struct db_pyhm *)pyhm_header->data;

  for (j = 0 ; j < pyhm_data->num_dohm ; j++) {
    dohm_data = (struct db_dohm *)pyhm_header->children[j]->data;

    if (dohm_data->type == 0x64) {
      wierd_dohm_header = pyhm_header->children[j];
      break;
    }
  }

  if (wierd_dohm_header == NULL)
    return -1;

  wierd_dohm_data = (struct db_wierd_dohm *) wierd_dohm_header->data;
  num_shown = wierd_dohm_data->shows[0].unk10[0];

  if (num_shown < 2 || num_shown <= cola || num_shown <= pos)
    return -EINVAL;

  finish = (cola > pos) ? pos : cola;

  for (j = (cola > pos) ? cola : pos ; j > finish ; j--) {
    struct __shows__ tmp = wierd_dohm_data->shows[j];
    
    wierd_dohm_data->shows[j] = wierd_dohm_data->shows[j-1];
    wierd_dohm_data->shows[j-1] = tmp;
  }

  return 0;
}

int db_playlist_column_list_shown (ipoddb_t *itunesdb, int playlist, int data_section, int **list) {
  tree_node_t *pyhm_header, *wierd_dohm_header = NULL;
  int j, num_shown, ret;

  struct db_wierd_dohm *wierd_dohm_data;
  struct db_pyhm *pyhm_data;
  struct db_dohm *dohm_data;

  if (list == NULL)
    return -EINVAL;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  pyhm_data = (struct db_pyhm *)pyhm_header->data;

  for (j = 0 ; j < pyhm_data->num_dohm ; j++) {
    dohm_data = (struct db_dohm *)pyhm_header->children[j]->data;

    if (dohm_data->type == 0x64) {
      wierd_dohm_header = pyhm_header->children[j];
      break;
    }
  }

  if (wierd_dohm_header == NULL)
    return -1;

  wierd_dohm_data = (struct db_wierd_dohm *) wierd_dohm_header->data;
  num_shown = wierd_dohm_data->shows[0].unk10[0];

  *list = calloc (num_shown, sizeof(int));

  for (j = 0 ; j < num_shown ; j++)
    (*list)[j] = wierd_dohm_data->shows[j].show;

  return num_shown;
}

int db_playlist_get_name (ipoddb_t *itunesdb, int playlist, int data_section, char **name) {
  tree_node_t *pyhm_header;
  tree_node_t *dohm_header = NULL;

  struct db_pyhm *pyhm_data;
  struct db_dohm *dohm_data = NULL;

  int i;
  int ret;

  db_log (itunesdb, 0, "db_playlist_get_name: entering...\n");
  
  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, data_section, &pyhm_header)) < 0)
    return ret;

  pyhm_data = (struct db_pyhm *)pyhm_header->data;

  for (i = 0 ; i < pyhm_data->num_dohm ; i++) {
    dohm_data = (struct db_dohm *)pyhm_header->children[i]->data;

    if (dohm_data->type == IPOD_TITLE) {
      dohm_header = pyhm_header->children[i];
      break;
    }
  }

  if (dohm_header == NULL)
    return -1;
  
  db_dohm_get_string (dohm_header, (u_int8_t **)name);

  db_log (itunesdb, 0, "db_playlist_get_name: complete\n");

  return 0;
}

int db_playlist_strip_indices (ipoddb_t *itunesdb) {
  db_playlist_strip_indices_ds (itunesdb, 2);
  db_playlist_strip_indices_ds (itunesdb, 3);
}

int db_playlist_strip_indices_ds (ipoddb_t *itunesdb, int data_section) {
  tree_node_t *pyhm_header, *dohm_header;
  struct db_dohm *dohm_data;
  struct db_pyhm *pyhm_data;

  int i, ret;

  if (itunesdb == NULL)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_strip_indices: entering...\n");
  
  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, 0, data_section, &pyhm_header)) < 0)
    return ret;
 
  pyhm_data = (struct db_pyhm *)pyhm_header->data;

  db_log (itunesdb, 0, "db_playlist_strip_indices: main playlist has %i dohms\n", pyhm_data->num_dohm);

  for (i = (pyhm_data->num_dohm - 1) ; i >= 0 ; i--) {
    dohm_data = (struct db_dohm *)pyhm_header->children[i]->data;

    if (dohm_data->type == 0x34) {
      db_pyhm_dohm_detach (pyhm_header, i, &dohm_header);
      
      db_free_tree (dohm_header);
    }
  }

  db_log (itunesdb, 0, "db_playlist_strip_indices: complete\n");

  return 0;
}

int db_playlist_add_indices (ipoddb_t *itunesdb) {
  db_playlist_add_indices_ds (itunesdb, 2);
  db_playlist_add_indices_ds (itunesdb, 3);
}

int db_playlist_add_indices_ds (ipoddb_t *itunesdb, int data_section) {
  tree_node_t *pyhm_header, *dohm_header;
  struct db_pyhm *pyhm_data;
  int sort_by[] = {IPOD_TITLE, IPOD_ALBUM, IPOD_ARTIST, IPOD_GENRE, IPOD_COMPOSER, -1};

  u_int32_t *tracks;
  u_int32_t num_tracks;

  int i, ret;

  if (itunesdb == NULL)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_add_indices: entering...\n");
  
  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, 0, data_section, &pyhm_header)) < 0)
    return ret;
 
  pyhm_data = (struct db_pyhm *)pyhm_header->data;

  for (i = 0 ; sort_by[i] > -1 ; i++) {
    if (db_tihm_get_sorted_indices (itunesdb, sort_by[i], &tracks, &num_tracks) < 0) 
      continue;

    if (db_dohm_index_create (&dohm_header, sort_by[i], num_tracks, tracks) == 0)
      db_pyhm_dohm_attach (pyhm_header, dohm_header);

    free (tracks);
  }

  db_log (itunesdb, 0, "db_playlist_add_indices: complete\n");

  return 0;
}

int db_playlist_get_podcast (ipoddb_t *itunesdb, int data_section, tree_node_t **pyhm_header) {
  tree_node_t *dshm_header;
  db_plhm_t *plhm_data;

  int i, ret;

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, &dshm_header, 0, data_section, pyhm_header)) < 0)
    return ret;

  for (i = 1 ; i < plhm_data->list_entries ; i++) {
    tree_node_t *pyhm_cell = dshm_header->children[1 + i];
    struct db_pyhm *pyhm_data = (struct db_pyhm *)pyhm_cell->data;

    if (pyhm_data->flags & 0x00ff0000) {
      if (pyhm_header)
	*pyhm_header = pyhm_cell;

      return i;
    }
  }

  return -1;
}
