/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.2a playlist.c
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

#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

/*
  db_playlist_retrieve_header:

  Internal function that saves a pointer to the database's plhm and playlist
  dshm to the pointers that are passed in.

  Arguments:
   itunesdb_t *itunesdb    - opened itunesdb
   tree_node  *plhm_header - pointer to location of desired pointer
   tree_node  *dshm_heade  - pointer to location of desired parent pointer.

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_retrieve_header (itunesdb_t *itunesdb, tree_node_t **plhm_header,
				 tree_node_t **dshm_header) {
  tree_node_t *temp;
  
  if (db_dshm_retrieve (itunesdb, &temp, 0x2) < 0)
    return -1;

  *plhm_header = temp->children[0];

  if (dshm_header) *dshm_header = temp;

  return 0;
}

/**
  db_playlist_number:

  Returns the number of playlists in the itunesdb.

  Arguments:
   itunesdb_t *itunesdb - open itunesdb

  Returns:
   < 1 on error
   number of playlists on success
**/
int db_playlist_number (itunesdb_t *itunesdb) {
  tree_node_t *plhm_header;
  struct db_plhm *plhm_data;

  if (itunesdb == NULL) return -1;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, NULL) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  return plhm_data->num_pyhm;
}

/**
  db_playlists_list:

  Returns a linked list containing the numbers and names of the playlists in
  the itunesdb.

  Arguments:
   itunesdb_t *itunesdb - opened itunesdb

  Returns:
   NULL on error
   pointer to head of linked list on success
**/
GList *db_playlist_list (itunesdb_t *itunesdb) {
  tree_node_t *plhm_header, *dshm_header, *pyhm_header, *dohm_header;
  struct pyhm *pyhm;
  struct db_plhm *plhm_data;
  struct db_dohm *dohm_data;

  GList *head, **current, *prev = NULL;

  int i;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0)
    return NULL;

  plhm_data = (struct db_plhm *) plhm_header->data;

  current = &head;
  for ( i = 1 ; i <= plhm_data->num_pyhm ; i++ ) {
    pyhm_header = dshm_header->children[i];
    dohm_header = pyhm_header->children[1];
    dohm_data   = (struct db_dohm *)dohm_header->data;

    *current = malloc (sizeof (GList));
    pyhm     = malloc (sizeof (struct pyhm));
    memset (pyhm, 0, sizeof (struct pyhm));

    pyhm->num = i - 1;
    pyhm->name= malloc (dohm_data->len / 2);
    memset (pyhm->name, 0, dohm_data->len / 2);

    unicode_to_char (pyhm->name, &dohm_header->data[0x28], dohm_data->len);

    (*current)->data = (void *)pyhm;
    
    (*current)->prev = prev;
    prev = *current;
    current = &(*current)->next;
  }

  return head;
}

/**
  db_playlist_list_free:

  Frees the memory used by a GList containing playlist information.

  Arguments:
   GList *head - head of allocated playlist list

  Returns:
   nothing, void function
**/
void db_playlist_list_free (GList *head) {
  struct pyhm *pyhm;

  while (head) {
    GList *tmp = head->next;

    if (head->data) {
      pyhm = (struct pyhm *)head->data;

      if (pyhm->name)
	free (pyhm->name);

      free (head->data);
    }
    
    free (head);
    head = tmp;
  }
}

/**
  db_playlist_list_songs:

  Stores an integer list of all the references contained in a playlist.

  Argument:
   itunesdb_t *itunesdb - opened itunesdb
   int         playlist - playlist index (0 is master)
   int       **list     - pointer to where the list should go

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_list_songs (itunesdb_t *itunesdb, int playlist, int **list) {
  tree_node_t *plhm_header, *dshm_header, *pyhm_header;

  struct db_plhm *plhm_data;
  struct db_pyhm *pyhm_data;
  struct db_pihm *pihm_data;

  int i;

  if (itunesdb == NULL || list == NULL) return -1;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist < 0 || playlist >= plhm_data->num_pyhm) return -1;

  pyhm_header = dshm_header->children[playlist + 1];
  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  *list = malloc (sizeof (int) * pyhm_data->num_pihm);

  for (i = 0 ; i < pyhm_data->num_pihm ; i++) {
    pihm_data = (struct db_pihm *)pyhm_header->children[i * 2 + 2]->data;
    (*list)[i] = pihm_data->reference;
  }

  return pyhm_data->num_pihm;
}

/**
  db_playlist_create:

   Creates a new playlist on the ipod with name.

  Arguments:
   itunesdb_t *itunesdb - Opened itunesdb
   char       *name     - Name of new playlist
   int         name_len - length of buffer name

  Returns:
   < 0 on error
     0 on success
**/
int db_playlist_create (itunesdb_t *itunesdb, char *name, int name_len) {
  tree_node_t *plhm_header, *new_pyhm, *new_dohm, *dshm_header;
  struct db_plhm *plhm;
  struct db_dohm *dohm;

  struct db_wierd_dohm *wierd_dohm_data;

  struct db_pyhm *pyhm;
  int unicode_len;
  char *unicode_data;

  int i;

  if (name == NULL) return -1;
  if (itunesdb == NULL) return -1;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0)
    return -1;

  unicode_check_and_copy (&unicode_data, &unicode_len, name, name_len);

  plhm = (struct db_plhm *) plhm_header->data;

  new_pyhm = (tree_node_t *) malloc (sizeof (tree_node_t));
  if (new_pyhm == NULL) {
    perror ("db_create_new_playlist|malloc");
    return errno;
  }

  if (db_pyhm_create (new_pyhm) < 0)
    return -1;

  pyhm = (struct db_pyhm *) new_pyhm->data;

  if (plhm->num_pyhm == 0) {
    pyhm->is_visible = 1;
    pyhm->unk0       = 0xb94613e8;
  }

  /* I dont know what this dohm entry does */
  new_dohm = (tree_node_t *) malloc (sizeof (tree_node_t));
  if (new_dohm == NULL) {
    perror ("db_create_new_playlist|malloc");
    return errno;
  }
  if (db_dohm_create_generic (new_dohm, 0x288, 0x64) < 0)
    return -1;

  wierd_dohm_data = (struct db_wierd_dohm *)new_dohm->data;

  /* i dont know what these two are about, but they need to be set like this */
  wierd_dohm_data->unk6 = 0x00010000;
  wierd_dohm_data->unk7 = 0x00000004;

  /* but, now I know what this is (cheer!) */
  wierd_dohm_data->shows[0].unk10[0] = 6;

  /* show playing column */
  wierd_dohm_data->shows[0].show = SHOW_PLAYING | 0x00120000;

  /* show a few more colums */
  for (i = 1; i < 6 ; i++)
    /* im just showing some usefull info (artist, bitrate, etc) */
    wierd_dohm_data->shows[i].show = (i + 0x1) | 0x007d0000; /* 7d pixel(?) width */

  db_attach (new_pyhm, new_dohm);

  /* create the title entry for the new playlist */
  new_dohm = (tree_node_t *) malloc (sizeof (tree_node_t));
  if (new_dohm == NULL) {
    perror ("db_create_new_playlist|malloc");
    return errno;
  }
  if (db_dohm_create_generic (new_dohm, 0x28 + unicode_len, IPOD_TITLE) < 0)
    return -1;

  dohm       = (struct db_dohm *)new_dohm->data;
  dohm->unk2 = 1;
  dohm->len  = unicode_len;

  memcpy (&new_dohm->data[0x28], unicode_data, unicode_len);

  db_attach (new_pyhm, new_dohm);

  /* this MUST be done last to avoid adding the sizes it's children twice */
  db_attach (dshm_header, new_pyhm);

  free (unicode_data);
  return plhm->num_pyhm++;
}

/**
  db_playlist_delete:
    
    Delete a playlist from the itunesdb.

  Arguments:
   itunesdb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is master)

  Returns:
   < 0 on error or when attempting to remove the master playlist
     0 on success
**/
int db_playlist_delete (itunesdb_t *itunesdb, int playlist) {
  tree_node_t *plhm_header, *dshm_header, *pyhm_header;
  struct db_plhm *plhm_data;

  if (itunesdb == NULL) return -1;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *)plhm_header->data;

  /* dont allow the deletion of the master playlist */
  if (playlist < 1 || playlist >= plhm_data->num_pyhm)
    return -1;

  db_detach (dshm_header, playlist + 1, &pyhm_header);
  db_free_tree (pyhm_header);
}

/**
  db_playlist_tihm_add:

    Adds a reference to song tihm_num to playlist.

  Arguments:
   itunesdb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is the master playlist)
   int         tihm_num - Song reference to remove

  Returns:
   < 0 on error
     0 on success
**/
int db_playlist_tihm_add (itunesdb_t *itunesdb, int playlist, int tihm_num) {
  tree_node_t *plhm_header, *pyhm_header, *dshm_header;
  tree_node_t *pihm, *dohm;

  struct db_plhm *plhm_data;
  struct db_pyhm *pyhm_data;

  if (itunesdb == NULL) return -1;
  
  if (db_tihm_retrieve (itunesdb, NULL, NULL, tihm_num) < 0)
    return -1;
    
  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  pyhm_header = dshm_header->children[playlist + 1];
  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  /* allocate memory */
  pihm = (tree_node_t *) malloc (sizeof (tree_node_t));
  if (pihm == NULL) {
    perror ("db_add_to_playlist|malloc");
    return -1;
  }

  dohm = (tree_node_t *) malloc (sizeof (tree_node_t));
  if (dohm == NULL) {
    perror ("db_add_to_playlist|malloc");
    return -1;
  }

  db_pihm_create (pihm, tihm_num, tihm_num);
  db_dohm_create_generic (dohm, 0x2c, 0x64);

  db_attach (pyhm_header, pihm);
  db_attach (pyhm_header, dohm);

  pyhm_data->num_pihm += 1;

  return 0;
}

/**
  db_playlist_tihm_remove:

    Removes the reference to song tihm_num from playlist.

  Arguments:
   itunesdb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is the master playlist)
   int         tihm_num - Song reference to remove

  Returns:
   < 0 on error
     0 on success
**/
int db_playlist_tihm_remove (itunesdb_t *itunesdb, int playlist,
			     int tihm_num) {
  tree_node_t *plhm_header, *pyhm_header, *dshm_header;
  tree_node_t *pihm, *dohm;

  struct db_plhm *plhm_data;
  struct db_pyhm *pyhm_data;

  int entry_num;

  if (itunesdb == NULL) return -1;
  
  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  pyhm_header = dshm_header->children[playlist + 1];
  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  /* search and destroy */
  entry_num = db_pihm_search (pyhm_header, tihm_num);

  db_detach (pyhm_header, entry_num, &pihm);
  db_detach (pyhm_header, entry_num, &dohm);

  db_free_tree (pihm);
  db_free_tree (dohm);

  pyhm_data->num_pihm -= 1;

  return 0;
}

/*
  db_playlist_clear:

    Clears all references from playlist.

  Arguments:
   itunesdb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is the master playlist)

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_clear (itunesdb_t *itunesdb, int playlist) {
  tree_node_t *plhm_header, *pyhm_header, *dshm_header;
  tree_node_t *pihm, *dohm;

  struct db_plhm *plhm_data;
  struct db_pyhm *pyhm_data;

  int entry_num;

  if (itunesdb == NULL) return -1;
  
  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  pyhm_header = dshm_header->children[playlist + 1];
  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  while (pyhm_data->num_pihm--) {
    db_detach (pyhm_header, 2, &pihm);
    db_detach (pyhm_header, 2, &dohm);

    db_free_tree (pihm);
    db_free_tree (dohm);
  }

  pyhm_data->num_pihm = 0;
}

/*
  db_playlist_fill:

    Fills a playlist with references to every song.

  Arguments:
   itunesdb_t *itunesdb - Opened itunesdb
   int         playlist - Playlist index (0 is the master playlist)

  Returns:
   < 0 on error
     0 on success
*/
int db_playlist_fill (itunesdb_t *itunesdb, int playlist) {
  tree_node_t *first_dshm, *tlhm_header, *tihm_header;

  struct db_tlhm *tlhm_data;
  struct db_tihm *tihm_data;

  int i;

  if (itunesdb == NULL || itunesdb->tree_root == NULL) return -1;

  db_playlist_clear (itunesdb, playlist);

  first_dshm = itunesdb->tree_root->children[0];
  tlhm_header= first_dshm->children[0];
  tlhm_data  = (struct db_tlhm *)tlhm_header->data;

  for (i = 0 ; i < tlhm_data->num_tihm ; i++) {
    tihm_data = (struct db_tihm *)first_dshm->children[i + 1]->data;
    db_playlist_tihm_add (itunesdb, playlist, tihm_data->identifier);
  }

  return 0;
}

int db_playlist_remove_all (itunesdb_t *itunesdb, int tihm_num) {
  int i, total;

  total = db_playlist_number (itunesdb);
  
  for (i = 0 ; i < total ; i++)
    db_playlist_tihm_remove (itunesdb, i, tihm_num);

  return 0;
}

int db_playlist_column_show (itunesdb_t *itunesdb, int playlist, int column, u_int16_t width) {
  tree_node_t *dshm_header, *wierd_dohm_header, *plhm_header;

  int num_shown;

  struct db_wierd_dohm *wierd_dohm_data;
  struct db_plhm *plhm_data;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0) {
    return -1;
  }

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  wierd_dohm_header = dshm_header->children[playlist + 1];
  wierd_dohm_data = (struct db_wierd_dohm *) wierd_dohm_header->data;
  num_shown = wierd_dohm_data->shows[0].unk10[0];

  if (num_shown > 36)
    return -1;

  wierd_dohm_data->shows[num_shown].show = column | width << 16;
  wierd_dohm_data->shows[0].unk10[0] = num_shown + 1;

  return 0;
}

int db_playlist_column_hide (itunesdb_t *itunesdb, int playlist, int column) {
  tree_node_t *dshm_header, *wierd_dohm_header, *plhm_header;
  int i, j;

  int num_shown;

  struct db_wierd_dohm *wierd_dohm_data;
  struct db_plhm *plhm_data;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0) {
    return -1;
  }

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  wierd_dohm_header = dshm_header->children[playlist + 1];
  wierd_dohm_data = (struct db_wierd_dohm *) wierd_dohm_header->data;
  num_shown = wierd_dohm_data->shows[0].unk10[0];

  if (num_shown == 0)
    return -1;

  for (i = 0 ; i < num_shown ; i++) {
    if (wierd_dohm_data->shows[i].show & 0xff == column)
      break;
  }

  if (i == num_shown)
    return -4;

  for (j = i + 1 ; j < num_shown ; j++)
    wierd_dohm_data->shows[j-1] = wierd_dohm_data->shows[j];

  wierd_dohm_data->shows[0].unk10[0] = num_shown - 1;;

  return 0;
}

/*
  first position is position 0
*/
int db_playlist_column_move (itunesdb_t *itunesdb, int playlist, int cola, int pos) {
  tree_node_t *dshm_header, *wierd_dohm_header, *plhm_header;
  int j;
  int finish;

  int num_shown;

  struct db_wierd_dohm *wierd_dohm_data;
  struct db_plhm *plhm_data;

  if (cola < 0 || pos < 0 || cola == pos) return -1;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) < 0) {
    return -1;
  }

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  wierd_dohm_header = dshm_header->children[playlist + 1];
  wierd_dohm_data = (struct db_wierd_dohm *) wierd_dohm_header->data;
  num_shown = wierd_dohm_data->shows[0].unk10[0];

  if (num_shown < 2 || num_shown <= cola || num_shown <= pos)
    return -1;

  finish = (cola > pos) ? pos : cola;

  for (j = (cola > pos) ? cola : pos ; j > finish ; j--) {
    struct __shows__ tmp = wierd_dohm_data->shows[j];
    
    wierd_dohm_data->shows[j] = wierd_dohm_data->shows[j-1];
    wierd_dohm_data->shows[j-1] = tmp;
  }

  return 0;
}
