/**
 *   (c) 2004-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 song_list.c
 *   
 *   Routines for working with an iTunesDB
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

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/uio.h>
#include <unistd.h>

#include <fcntl.h>

#include <errno.h>

#include "itunesdbi.h"

/**
  db_remove:

   Deletes an entry from the song list, then deletes the referance (if any)
  from all existing playlists.

  Arguments:
   ipoddb_t *itunesdb - opened itunesdb
   u_int32_t   tihm_num - song reference which to remove

  Returns:
   < 0 on error
     0 on success
**/
int db_song_remove (ipoddb_t *itunesdb, u_int32_t tihm_num) {
  tree_node_t *parent, *entry;
  struct db_tlhm *tlhm;
  int entry_num;

  if (itunesdb == NULL || itunesdb->tree_root == NULL || itunesdb->type != 0)
    return -EINVAL;

  entry_num = db_tihm_retrieve (itunesdb, &entry, &parent, tihm_num);

  if (entry_num < 0) {
    db_log (itunesdb, 0, "db_remove %i: no song found\n", tihm_num);

    return entry_num;
  }

  tlhm = (struct db_tlhm *)parent->children[0]->data;
  tlhm->num_tihm -= 1;

  /* remove the entry */
  db_detach (parent, entry_num, &entry);
  db_free_tree (entry);

  /* remove from all playlists */
  db_playlist_remove_all (itunesdb, tihm_num);

  return 0;
}

/**
  db_add:

   Adds a song to the song list, then adds a reference to that song to the
  master playlist.

  Arguments:
   ipoddb_t *itunesdb - Opened iTunesDB
   tihm_t     *tihm     - A tihm_t structure populated with artist, album, etc.

  Returns:
   < 0 on error
   >=0 on success
*/
int db_song_add (ipoddb_t *itunesdb, ipoddb_t *artworkdb, char *path,
		 u_int8_t *mac_path, int stars, int show) {
  tree_node_t *dshm_header, *new_tihm_header;

  struct db_tlhm *tlhm_data;
  int tihm_num, ret;

  tihm_t tihm;

  if (itunesdb == NULL || path == NULL || mac_path == NULL || itunesdb->type != 0)
    return -EINVAL;

  /* find the song list */
  if ((ret = db_dshm_retrieve (itunesdb, &dshm_header, 1)) < 0)
    return ret;

  if (db_lookup (itunesdb, IPOD_PATH, mac_path) > -1)
    return -1; /* A song already exists in the database with this path */
  
  /* Set the new tihm entry's number to 1 + the previous one */
  tihm_num = itunesdb->last_entry + 1;

  if ((ret = tihm_fill_from_file (&tihm, path, mac_path, stars, tihm_num)) < 0) {
    db_log (itunesdb, ret, "Could not fill tihm structure from file: %s.\n",
	    path);

    return ret;
  }

  if ((ret = db_tihm_create (&new_tihm_header, &tihm, itunesdb->flags)) < 0) {
    db_log (itunesdb, ret, "Could not create tihm entry\n");
    free (new_tihm_header);
    return ret;
  }

  if (artworkdb && tihm.image_data)
    db_photo_add (artworkdb, tihm.image_data, tihm.image_size, tihm.artwork_id);

  tihm_free (&tihm);
  
  new_tihm_header->parent = dshm_header;
  db_attach (dshm_header, new_tihm_header);

  if (show != 0)
    db_song_unhide(itunesdb, tihm_num);

  /* everything was successfull, increase the tihm count in the tlhm header */
  tlhm_data = (struct db_tlhm *)dshm_header->children[0]->data;
  tlhm_data->num_tihm += 1;

  itunesdb->last_entry++;

  return tihm_num;
}

/*
  db_hide:

  Removes the reference to song tihm_num from the master playlist.

  Returns:
    < 0 on error
      0 on success
*/
int db_song_hide (ipoddb_t *itunesdb, u_int32_t tihm_num) {
  return db_playlist_tihm_remove (itunesdb, 0, tihm_num);
}

/*
  db_unhide:

  Adds a reference to song tihm_num to the master playlist.

  Returns:
   < 0 on error
     0 on success
*/
int db_song_unhide (ipoddb_t *itunesdb, u_int32_t tihm_num) {
  return db_playlist_tihm_add (itunesdb, 0, tihm_num);
}

/**
  db_modify_eq:

   Modifies (or adds) an equilizer setting of(to) a song entry.

  Arguments:
   ipoddb_t *itunesdb - opened itunesdb
   u_int32_t  *tihm_num - song entry to modify
   int         eq       - reference number of the eq setting

  Returns:
   < 0 on error
     0 on success
**/
int db_song_modify_eq (ipoddb_t *itunesdb, u_int32_t tihm_num, int eq) {
  tree_node_t *tihm_header, *dohm_header;
  struct db_tihm *tihm_data = NULL;
  int ret;

  if ((ret = db_tihm_retrieve (itunesdb, &tihm_header, NULL, tihm_num)) < 0) {
    db_log (itunesdb, ret, "db_song_modify_eq %i: no song found\n", tihm_num);

    return ret;
  }

  /* see if an equilizer entry already exists */
  if ((ret = db_dohm_retrieve (tihm_header, &dohm_header, 0x7)) < 0) {
    tihm_data = (struct db_tihm *) tihm_header->data;
    tihm_data->num_dohm ++;
  } else {
    dohm_header->parent = NULL;
    db_free_tree (dohm_header);
  }

  if ((ret = db_dohm_create_eq (&dohm_header, eq)) < 0)
    return ret;

  db_attach (tihm_header, dohm_header);

  return 0;
}

int db_song_set_artwork (ipoddb_t *itunesdb, u_int32_t tihm_num,
			 u_int64_t iihm_id) {
  tree_node_t *tihm_header;
  struct db_tihm *tihm_data;
  int ret;

  if ((ret = db_tihm_retrieve (itunesdb, &tihm_header, NULL, tihm_num)) < 0) {
    db_log (itunesdb, ret, "db_song_set_artwork %i: no song found\n", tihm_num);

    return ret;
  }

  tihm_data = (struct db_tihm *)tihm_header->data;

  tihm_data->iihm_id = iihm_id;

  return 0;
}

/**
  db_song_list:

   Returns a linked list of the songs that are currently in the song list
  of the iTunesDB.

  Arguments:
   ipoddb_t *itunesdb - opened itunesdb

  Returns:
   NULL on error
   ptr  on success
**/
int db_song_list (ipoddb_t *itunesdb, GList **head) {
  tree_node_t *dshm_header, *tihm_header, *tlhm_header;
  struct db_tlhm *tlhm_data;
  int i, *iptr;
  int ret;

  if (head == NULL || itunesdb == NULL || itunesdb->type != 0)
    return -EINVAL;

  *head = NULL;

  /* get the tree node containing the song list */
  if ((ret = db_dshm_retrieve (itunesdb, &dshm_header, 0x1)) < 0)
    return ret;

  tlhm_header = dshm_header->children[0];
  tlhm_data   = (struct db_tlhm *)tlhm_header->data;

  /* cant create a song list if there are no songs */
  if (tlhm_data->num_tihm == 0)
    return -1;

  for (i = dshm_header->num_children - 1 ; i > 0 ; i--) {
    tihm_header = dshm_header->children[i];
    iptr = (int *)tihm_header->data;

    /* only add tree nodes containing tihm entries
       to the new song list */
    if (iptr[0] != TIHM)
      continue;

    *head = g_list_prepend (*head, db_tihm_fill (tihm_header));
  }

  return 0;
}

/**
  db_song_list_free:

   Frees the memory that has been allocated to a song list. I strongly
  recomend you call this to clean up at the end of execution.

  Arguments:
   GList *head - pointer to allocated song list

  Returns:
   nothing, void function
**/
void db_song_list_free (GList **head) {
  GList *tmp;

  for (tmp = g_list_first (*head) ; tmp ; tmp = g_list_next (tmp)) {
    tihm_free ((tihm_t *)(tmp->data));
    free (tmp->data);
  }

  g_list_free (*head);

  *head = NULL;
}
