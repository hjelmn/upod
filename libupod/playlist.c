/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.2 playlist.c
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "itunesdbi.h"

/*
  db_playlist_retrieve:

  Internal function that saves a pointer to the database's plhm and playlist
  dshm to the pointers that are passed in.

  Arguments:
   ipoddb_t *itunesdb         - opened itunesdb
   struct db_plhm **plhm_data - pointer to location to store plhm data
   tree_node  **dshm_header   - pointer to location of desired parent pointer.
   int playlist               - playlist to return
   tree_node  **pyhm_header   - pointer to location to store playlist header

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_retrieve (ipoddb_t *itunesdb, struct db_plhm **plhm_data,
			  tree_node_t **dshm_header, int playlist, tree_node_t **pyhm_header) {
  tree_node_t *temp, *plhm_header;
  struct db_plhm *plhm_data_loc;
  int ret;

  if (itunesdb == NULL || itunesdb->type != 0 || (playlist < 0 && pyhm_header == NULL))
    return -EINVAL;

  if ((ret = db_dshm_retrieve (itunesdb, &temp, 0x2)) < 0)
    return ret;

  plhm_header = temp->children[0];

  plhm_data_loc = (struct db_plhm *)temp->children[0]->data;

  /* Store the pointers if storage was passed in */
  if (plhm_data != NULL)
    *plhm_data = plhm_data_loc;

  if (dshm_header != NULL)
    *dshm_header = temp;

  if (pyhm_header != NULL) {
    if (playlist > (plhm_data_loc->num_pyhm - 1))
      return -EINVAL;

    *pyhm_header = temp->children[playlist + 1];
  }

  return 0;
}

/**
  db_playlist_number:

  Returns the number of playlists in the itunesdb.

  Arguments:
   ipoddb_t *itunesdb - open itunesdb

  Returns:
   < 1 on error
   number of playlists on success
**/
int db_playlist_number (ipoddb_t *itunesdb) {
  struct db_plhm *plhm_data;
  int ret;

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, NULL, 0, NULL)) < 0)
    return ret;

  return plhm_data->num_pyhm;
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
int db_playlist_list (ipoddb_t *itunesdb, GList **head) {
  struct db_plhm *plhm_data;
  u_int8_t *temp;

  int i, ret;

  if (head == NULL)
    return -EINVAL;
  else
    *head = NULL;

  db_log (itunesdb, 0, "db_playlist_list: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, NULL, 0, NULL)) < 0)
    return ret;

  for ( i = plhm_data->num_pyhm - 1 ; i >= 0 ; i--) {
    struct pyhm *pyhm;

    if ((ret = db_playlist_get_name (itunesdb, i, &temp)) < 0)
      continue;

    pyhm     = calloc (1, sizeof (struct pyhm));
    pyhm->num  = i - 1;
    pyhm->name = temp;
    pyhm->name_len = strlen (temp);

    *head = g_list_prepend (*head, pyhm);
  }

  db_log (itunesdb, 0, "db_playlist_list: complete\n");

  return 0;
}

/**
  db_playlist_list_free:

  Frees the memory used by a GList containing playlist information.

  Arguments:
   GList **head - pointer to head of allocated playlist list

  Returns:
   nothing, void function
**/
void db_playlist_list_free (GList **head) {
  GList *tmp;
  if (head == NULL)
    return;

  for (tmp = g_list_first (*head) ; tmp ; tmp = g_list_next (tmp)) {
    struct pyhm *pyhm = (struct pyhm *)tmp->data;

    free (pyhm->name);
    free (tmp->data);
  }

  g_list_free (*head);

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
int db_playlist_song_list (ipoddb_t *itunesdb, int playlist, GList **head) {
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

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
    return ret;

  pyhm_data = (struct db_pyhm *) pyhm_header->data;

  if (pyhm_data->num_pihm > 0) {
    num_entries = (pyhm_header->num_children - 2)/2;
    
    for (i = num_entries-1 ; i >= 0 ; i--) {
      pihm_data = (struct db_pihm *)pyhm_header->children[i * 2 + 2]->data;

      (*head) = g_list_prepend (*head, (void *)pihm_data->reference);
    }
  }

  db_log (itunesdb, 0, "db_playlist_song_list: complete\n");

  return num_entries;
}

void db_playlist_song_list_free (GList **head) {
  if (head == NULL)
    return;

  g_list_free (*head);
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
   < 0 on error
     0 on success
**/
int db_playlist_create (ipoddb_t *itunesdb, char *name, int name_len) {
  tree_node_t *new_pyhm, *new_dohm, *dshm_header;
  struct db_plhm *plhm_data;

  int ret;

  dohm_t dohm;

  if (name == NULL || name_len == 0)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_create: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, &dshm_header, 0, NULL)) < 0)
    return ret;

  to_unicode (&dohm.data, &dohm.size, name, name_len, "UTF-8");

  if ((ret = db_pyhm_create (&new_pyhm, plhm_data->num_pyhm == 0)) < 0)
    return ret;

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
  if ((ret = db_dohm_create (&new_dohm, dohm, 16)) < 0)
    return ret;

  db_pyhm_dohm_attach (new_pyhm, new_dohm);

  /* this MUST be done last to avoid adding the sizes it's children twice */
  db_attach (dshm_header, new_pyhm);

  free (dohm.data);

  db_log (itunesdb, 0, "db_playlist_create: complete\n");

  return plhm_data->num_pyhm++;
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
int db_playlist_rename (ipoddb_t *itunesdb, int playlist, char *name, int name_len) {
  tree_node_t *dohm_header, *pyhm_header;

  struct db_dohm *dohm_data;
  struct db_pyhm *pyhm_data;

  int i, ret;

  dohm_t dohm;

  if (name == NULL || name_len == 0)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_rename: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
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

  to_unicode (&dohm.data, &dohm.size, name, name_len, "UTF-8");
  dohm.type = IPOD_TITLE;

  if ((ret = db_dohm_create (&dohm_header, dohm, 16)) < 0)
    return ret;

  /* no need to worry about modifying the dohm count since it won't change */
  db_detach (pyhm_header, i, &dohm_header);
  db_free_tree (dohm_header);

  db_attach_at (pyhm_header, dohm_header, i);

  free (dohm.data);

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
int db_playlist_delete (ipoddb_t *itunesdb, int playlist) {
  tree_node_t *dshm_header, *pyhm_header;
  struct db_plhm *plhm_data;

  int ret;

  /* Can't delete the main playlist */
  if (playlist == 0)
    return -EINVAL;

  db_log (itunesdb, 0, "db_playlist_delete: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, &dshm_header, playlist, &pyhm_header)) < 0)
    return ret;

  plhm_data->num_pyhm--;

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
int db_playlist_tihm_add (ipoddb_t *itunesdb, int playlist, int tihm_num) {
  tree_node_t *pyhm_header, *pihm_header, *dohm_header;
  struct db_pyhm *pyhm_data;

  int entry_num, ret;

  db_log (itunesdb, 0, "db_playlist_tihm_add: entering...\n");

  /* make sure the tihm exists in the database before continuing */
  if ((ret = db_tihm_retrieve (itunesdb, NULL, NULL, tihm_num)) < 0)
    return ret;
    
  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
    return ret;

  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  /* check if the reference already exists in this playlist */
  entry_num = db_pihm_search (pyhm_header, tihm_num);

  if (entry_num == -1) {
    db_pihm_create (&pihm_header, tihm_num, tihm_num + 1);
    db_dohm_create_pihm (&dohm_header, tihm_num + 1);
    
    db_attach (pyhm_header, pihm_header);
    db_attach (pyhm_header, dohm_header);
    
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
int db_playlist_tihm_remove (ipoddb_t *itunesdb, int playlist, int tihm_num) {
  tree_node_t *pyhm_header, *pihm_header, *dohm_header;
  struct db_pyhm *pyhm_data;

  int entry_num, ret;

  db_log (itunesdb, 0, "db_playlist_tihm_remove: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
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
int db_playlist_clear (ipoddb_t *itunesdb, int playlist) {
  tree_node_t *pyhm_header, *pihm_header, *dohm_header;

  struct db_pyhm *pyhm_data;

  int ret;
  
  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
    return ret;

  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  while (pyhm_data->num_pihm--) {
    db_detach (pyhm_header, pyhm_data->num_dohm, &pihm_header);
    db_detach (pyhm_header, pyhm_data->num_dohm, &dohm_header);

    db_free_tree (pihm_header);
    db_free_tree (dohm_header);
  }

  return 0;
}

/*
  db_playlist_fill:

    Fills a playlist with references to every song.

  Arguments:
   ipoddb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is the master playlist)

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_fill (ipoddb_t *itunesdb, int playlist) {
  tree_node_t *track_dshm, *tlhm_header;

  struct db_tlhm *tlhm_data;
  struct db_tihm *tihm_data;

  int i, ret;

  if (itunesdb == NULL || itunesdb->type != 0)
    return -EINVAL;
  
  if ((ret = db_dshm_retrieve (itunesdb, &track_dshm, 1)) < 0)
    return -EINVAL;

  if ((ret = db_playlist_clear (itunesdb, playlist)) < 0)
    return ret;

  tlhm_header= track_dshm->children[0];
  tlhm_data  = (struct db_tlhm *)tlhm_header->data;

  for (i = 0 ; i < tlhm_data->num_tihm ; i++) {
    tihm_data = (struct db_tihm *)track_dshm->children[i + 1]->data;

    db_playlist_tihm_add (itunesdb, playlist, tihm_data->identifier);
  }

  return 0;
}

/* remove a track reference from every playlist */
int db_playlist_remove_all (ipoddb_t *itunesdb, int tihm_num) {
  int i, total;

  if ((total = db_playlist_number (itunesdb)) < 0)
    return total;
  
  for (i = 0 ; i < total ; i++)
    db_playlist_tihm_remove (itunesdb, i, tihm_num);

  return 0;
}

int db_playlist_column_show (ipoddb_t *itunesdb, int playlist, int column, u_int16_t width) {
  tree_node_t *pyhm_header;

  int ret;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
    return ret;

  return db_dohm_itunes_show (pyhm_header->children[0], column, width);
}

int db_playlist_column_hide (ipoddb_t *itunesdb, int playlist, int column) {
  tree_node_t *pyhm_header;

  int ret;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
    return ret;

  return db_dohm_itunes_hide (pyhm_header, column);
}

/*
  first position is position 0
*/
int db_playlist_column_move (ipoddb_t *itunesdb, int playlist, int cola, int pos) {
  tree_node_t *pyhm_header, *wierd_dohm_header = NULL;
  int num_shown, ret, j, finish;

  struct db_wierd_dohm *wierd_dohm_data;
  struct db_pyhm *pyhm_data;
  struct db_dohm *dohm_data;

  if (cola < 0 || pos < 0 || cola == pos)
    return -EINVAL;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
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

int db_playlist_column_list_shown (ipoddb_t *itunesdb, int playlist, int **list) {
  tree_node_t *pyhm_header, *wierd_dohm_header = NULL;
  int j, num_shown, ret;

  struct db_wierd_dohm *wierd_dohm_data;
  struct db_pyhm *pyhm_data;
  struct db_dohm *dohm_data;

  if (list == NULL)
    return -EINVAL;

  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
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

int db_playlist_get_name (ipoddb_t *itunesdb, int playlist, u_int8_t **name) {
  tree_node_t *pyhm_header;
  tree_node_t *dohm_header = NULL;

  struct db_pyhm *pyhm_data;
  struct db_dohm *dohm_data = NULL;

  struct string_header_16 *string_header;

  int i;
  size_t length = -1;

  int ret;

  db_log (itunesdb, 0, "db_playlist_get_name: entering...\n");
  
  if ((ret = db_playlist_retrieve (itunesdb, NULL, NULL, playlist, &pyhm_header)) < 0)
    return ret;

  pyhm_data = (struct db_pyhm *)pyhm_header->data;

  for (i = 0 ; i < pyhm_data->num_dohm ; i++) {
    dohm_data = (struct db_dohm *)pyhm_header->children[i]->data;

    if (dohm_data->type == IPOD_TITLE) {
      dohm_header = pyhm_header->children[i];
      break;
    }
  }

  if (dohm_header != NULL) {
    string_header = (struct string_header_16 *)&(dohm_header->data[0x18]);
    if (string_header->format == 0)
      unicode_to_utf8 (name, &length, (u_int16_t *)&dohm_header->data[0x28],
		       string_header->string_length);
    else if (string_header->format == 1) {
      *name = calloc (string_header->string_length + 1, 1);
      memmove (*name, &dohm_header->data[0x28], string_header->string_length);
      length = string_header->string_length;
    }
  }

  db_log (itunesdb, 0, "db_playlist_get_name: complete\n");

  return length;
}
