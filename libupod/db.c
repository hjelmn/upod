/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.2a-29 db.c
 *
 *   Routines for reading/writing the iPod's iTunesDB.
 *
 *   Quick Note: I use the words: entry and record interchangably.
 *    Eventually I will clean this up and try to be a little more consistant
 *    with my language, so keep that in mind while reading this code.
 *
 *   Changes:
 *    (7-8-2002)
 *     - modified to fit with ipod-on-linux project
 *    (sometime after):
 *     - added function to return song list
 *     - added functions for cleaning up memory
 *     - commented bottom half of code in db.c
 *     - check db_lookup.c for code for looking up song entries.
 *    (5-30-2002):
 *     - iTunesDB is now handled as a tree
 *     - db_remove implemented.
 *     - db_add implemented.
 *    (older):
 *     - handles byte conversion for powerpc in both directions
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

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <errno.h>

static int db_debug = 1;

/*
  db_size_tree:

    Internal utility function for acumulating the total size of
  the database below an entry.
*/
static int db_size_tree (tree_node_t *ptr) {
  int i, size = ptr->size;

  for (i = 0 ; i < ptr->num_children ; i++)
    size += db_size_tree (ptr->children[i]);

  return size;
}

/*
  db_free_tree:

  Function to recursively free a tree.
*/
void db_free_tree (tree_node_t *ptr) {
  if (ptr == NULL) return;

  if (ptr->num_children) {
    while (--ptr->num_children > -1)
      db_free_tree(ptr->children[ptr->num_children]);

    free(ptr->children);
  }

  free(ptr->data);
  free(ptr);
}

/**
  db_free:

  Frees the memory that is allocated to an itunesdb

  Arguments:
   itunesdb_t *itunesdb - opened itunesdb

  Returns:
   nothing, void function
**/
void db_free (itunesdb_t *itunesdb) {
  if (itunesdb == NULL) return;

  db_free_tree(itunesdb->tree_root);
}

/*
  db_build_tree:

  Internal function. 

  Purpose is to build up the iTunesDB tree from a buffer.
*/
static tree_node_t *db_build_tree (size_t *bytes_read,
					tree_node_t *parent,
					char **buffer) {
  tree_node_t *tnode_0;
  int *iptr = (int *)*buffer;
  int i;

  int current_bytes_read = *bytes_read;
  int entry_size, cell_size, copy_size;

  struct db_dohm *dohm_data;

  tnode_0 = calloc (1, sizeof(tree_node_t));

  if (tnode_0 == NULL) {
    perror("db_build_tree|calloc");
    exit(1);
  }

  memset (tnode_0, 0, sizeof(tree_node_t));

  tnode_0->parent = parent;

  bswap_block(*buffer, sizeof(u_int32_t), 3);

  entry_size = iptr[2];
  cell_size  = iptr[1];

  if (iptr[0] == DOHM) {
    dohm_data = (struct db_dohm *)*buffer;
    
    /* swap the header then the data (if needed) */
    bswap_block(&((*buffer)[0xc]), 4, 7);
    bswap_block(&((*buffer)[0x28]), 2, dohm_data->len / 2);

    copy_size = entry_size;
  } else {
    bswap_block(&(*buffer)[0xc], sizeof(u_int32_t), cell_size/4 - 3);
    
    copy_size = cell_size;
  }
  
  tnode_0->num_children = 0;
  tnode_0->size = copy_size;
  tnode_0->data = calloc (copy_size, 1);
  memcpy (tnode_0->data, *buffer, copy_size);
  
  *buffer     += copy_size;
  *bytes_read += copy_size;

  iptr = (int *)tnode_0->data;

  /* becuase the tlhm structures second field is not a total size,
     do not do anything more with it */
  if (cell_size == entry_size ||
      iptr[0] == TLHM ||
      iptr[0] == DOHM ||
      iptr[0] == PLHM )
    goto dbbt_done;

  tnode_0->children = calloc(1, sizeof(tree_node_t *));
  
  if (tnode_0->children == NULL) {
    perror("db_build_tree|calloc");
    exit(1);
  }

  while (*bytes_read - current_bytes_read < entry_size) {
    tnode_0->children = realloc(tnode_0->children, 
				++(tnode_0->num_children) *
				sizeof(tree_node_t *));
    
    if (tnode_0->children == NULL) {
      perror("db_build_tree|realloc");
      exit(1);
    }

    tnode_0->children[tnode_0->num_children-1] = db_build_tree(bytes_read,
							       tnode_0,
							       buffer);
  }

 dbbt_done:
  return tnode_0;
}

/**
  db_load:

  Function that loads an itunesdb from a file. If volume is NULL then
  the database is loaded from a unix file described by path.

  Arguments:
   itunesdb_t *itunesdb - pointer to where the itunesdb should go.
   HFSPlus    *volume   - the HFSPlus volume to load from
   char       *path     - the path (including partition name) to where the database is

  Returns:
   < 0 on error
     0 on success
**/
int db_load (itunesdb_t *itunesdb, char *path) {
  int iTunesDB_fd;
  char *buffer;
  int ibuffer[3];
  int ret;

  int *tmp;

  size_t bytes_read  = 0;

  if (path == NULL || strlen(path) == 0) return -1;

  if ((iTunesDB_fd = open (path, O_RDONLY)) < 0) {
    UPOD_DEBUG (errno, "Could not open iTunesDB %s\n", path);
    return -1;
  }

  read (iTunesDB_fd, ibuffer, 12);

  bswap_block((char *)ibuffer, 4, 3);

  if (ibuffer[0] != DBHM) {
    close (iTunesDB_fd);

    return -1;
  }
  
  buffer = (char *)calloc(ibuffer[2], 1);
  if (buffer == NULL) UPOD_ERROR(errno, "Could not allocate memory\n");

  /* keep track of where buffer starts */
  tmp = (int *)buffer;

  if ((ret = read (iTunesDB_fd, buffer + 12, ibuffer[2] - 12)) <
      (ibuffer[2] - 12)) {
    UPOD_ERROR(errno, "Short read: %i bytes wanted, %i read\n", ibuffer[2],
	       ret);
    
    free(buffer);
    close(iTunesDB_fd);
    return -1;
  }

  bswap_block((char *)ibuffer, 4, 3);
  memcpy (buffer, ibuffer, 12);
  
  close (iTunesDB_fd);

  itunesdb->tree_root = db_build_tree(&bytes_read, NULL, &buffer);

  free(tmp);

  return bytes_read;
}

static int db_write_tree (int fd, tree_node_t *entry) {
  static int ret;
  int i, swap, length;
  struct db_dohm *dohm_data;

#if BYTE_ORDER == BIG_ENDIAN
  dohm_data = (struct db_dohm *) entry->data;

  if (dohm_data->dohm == DOHM) {
    if (entry->size == 0x288) {
      swap = 0x288/4;
      length = 0;
    } else {
      swap = 10;
      length = dohm_data->len/2;
      bswap_block (&entry->data[0x28], 2, length);
    }
  } else
    swap = ((int *)entry->data)[1]/4;

  bswap_block(entry->data, 4, swap);
#endif

  ret += write (fd, entry->data, entry->size);

#if BYTE_ORDER == BIG_ENDIAN
  if (dohm_data->dohm == DOHM)
    bswap_block (&entry->data[0x28], 2, length);
#endif

  bswap_block(entry->data, 4, swap);

  for (i = 0 ; i < entry->num_children ; i++)
    db_write_tree (fd, entry->children[i]);

  return ret;
}

int db_write (itunesdb_t itunesdb, char *path) {
  int fd;

  int ret;
  int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  if (itunesdb.tree_root == NULL) return -1;

  if ((fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, perms)) < 0) {
    perror("db_write_unix");
    return errno;
  }

  ret = db_write_tree (fd, itunesdb.tree_root);
  
  close (fd);
  
  return ret;
}


/*
  db_hide:

  Removes the reference to song tihm_num from the master playlist.

  Returns:
    < 0 on error
      0 on success
*/
int db_hide (itunesdb_t *itunesdb, u_int32_t tihm_num) {
  return db_playlist_tihm_remove (itunesdb, 0, tihm_num);
}

/*
  db_unhide:

  Adds a reference to song tihm_num to the master playlist.

  Returns:
   < 0 on error
     0 on success
*/
int db_unhide (itunesdb_t *itunesdb, u_int32_t tihm_num) {
  return db_playlist_tihm_add (itunesdb, 0, tihm_num);
}

/**
  db_remove:

   Deletes an entry from the song list, then deletes the referance (if any)
  from all existing playlists.

  Arguments:
   itunesdb_t *itunesdb - opened itunesdb
   u_int32_t   tihm_num - song reference which to remove

  Returns:
   < 0 on error
     0 on success
**/
int db_remove (itunesdb_t *itunesdb, u_int32_t tihm_num) {
  tree_node_t *parent, *entry;
  struct db_tlhm *tlhm;
  int size, entry_num;

  if (itunesdb == NULL || itunesdb->tree_root == NULL) return -1;

  entry_num = db_tihm_retrieve (itunesdb, &entry, &parent, tihm_num);

  if (entry_num < 0) {
    UPOD_DEBUG(0, "db_remove %i: no song found\n", tihm_num);

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
   itunesdb_t *itunesdb - Opened iTunesDB
   tihm_t     *tihm     - A tihm_t structure populated with artist, album, etc.

  Returns:
   < 0 on error
     0 on success
*/
int db_add (itunesdb_t *itunesdb, char *path, char *mac_path, int mac_path_len, int stars) {
  tree_node_t *dshm_header, *new_tihm_header, *root;

  struct db_tlhm *tlhm_data;
  int tihm_num;

  /* find the song list */
  if (db_dshm_retrieve (itunesdb, &dshm_header, 1) < 0)
    return -1;

  /* allocate memory for the new tree node */
  new_tihm_header = (tree_node_t *)calloc(1, sizeof(tree_node_t));
  new_tihm_header->parent = dshm_header;
  
  tihm_num = db_tihm_create (new_tihm_header, path, mac_path, mac_path_len, stars);
  db_attach (dshm_header, new_tihm_header);

  db_unhide(itunesdb, tihm_num);

  /* everything was successfull, increase the tihm count in the tlhm header */
  tlhm_data = (struct db_tlhm *)dshm_header->children[0]->data;
  tlhm_data->num_tihm += 1;

  return 0;
}

/**
  db_modify_eq:

   Modifies (or adds) an equilizer setting of(to) a song entry.

  Arguments:
   itunesdb_t *itunesdb - opened itunesdb
   u_int32_t  *tihm_num - song entry to modify
   int         eq       - reference number of the eq setting

  Returns:
   < 0 on error
     0 on success
**/
int db_modify_eq (itunesdb_t *itunesdb, u_int32_t tihm_num, int eq) {
  tree_node_t *tihm_header, *dohm_header;
  struct db_tihm *tihm_data;

  if (db_tihm_retrieve (itunesdb, &tihm_header, NULL, tihm_num) < 0) {
    UPOD_DEBUG(-2, "db_song_modify_eq %i: no song found\n", tihm_num);

    return -2;
  }

  /* see if an equilizer entry already exists */
  if (db_dohm_retrieve (tihm_header, &dohm_header, 0x7) < 0) {
    dohm_header = calloc (1, sizeof(tree_node_t));

    if (dohm_header == NULL) {
      perror ("db_song_modify_eq|calloc");
      return -1;
    }

    memset (dohm_header, 0, sizeof(tree_node_t));

    db_attach (tihm_header, dohm_header);

    tihm_data = (struct db_tihm *) tihm_header->data;
    tihm_data->num_dohm ++;
  }

  if (db_dohm_create_eq (dohm_header, eq) < 0)
    return -1;

  return 0;
}

/**
  db_song_list:

   Returns a linked list of the songs that are currently in the song list
  of the iTunesDB.

  Arguments:
   itunesdb_t *itunesdb - opened itunesdb

  Returns:
   NULL on error
   ptr  on success
**/
GList *db_song_list (itunesdb_t *itunesdb) {
  GList **ptr, *head;
  tree_node_t *dshm_header, *tihm_header, *tlhm_header;
  struct db_tlhm *tlhm_data;
  int i, *iptr;

  struct db_tihm *tihm_data;

  /* get the tree node containing the song list */
  if (db_dshm_retrieve (itunesdb, &dshm_header, 0x1) < 0)
    return NULL;

  tlhm_header = dshm_header->children[0];
  tlhm_data   = (struct db_tlhm *)tlhm_header->data;

  /* cant create a song list if there are no songs */
  if (tlhm_data->num_tihm == 0)
    return NULL;

  ptr  = &(head);
  for (i = 1 ; i < dshm_header->num_children ; i++) {
    tihm_header = dshm_header->children[i];
    iptr = (int *)tihm_header->data;

    /* only add tree nodes containing tihm entries
       to the new song list */
    if (iptr[0] != TIHM)
      continue;

    *ptr  = (GList *) calloc (1, sizeof(GList));

    (*ptr)->data = (void *) db_tihm_fill (tihm_header);
    (*ptr)->next = NULL;

    ptr = &((*ptr)->next);
  }

  return head;
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
void db_song_list_free (GList *head) {
  GList *tmp;
  while (head) {
    tmp = head->next;

    tihm_free ((tihm_t *)head->data);

    free(head);
    head = tmp;
  }
}

int db_attach (tree_node_t *parent, tree_node_t *new_child) {
  int size;
  tree_node_t *tmp;

  if (parent == NULL || new_child == NULL) return -1;

  /* allocate memory for the new child pointer */
  if (parent->num_children++ == 0)
    parent->children = calloc (1, sizeof(tree_node_t *));
  else
    parent->children = realloc (parent->children, parent->num_children * 
				sizeof (tree_node_t *));

  /* add the new child node onto this node and set its parent
   pointer accordingly */
  new_child->parent = parent;
  parent->children[parent->num_children - 1] = new_child;

  /* adjust tree sizes */
  size = db_size_tree (new_child);

  for (tmp = parent ; tmp ; tmp = tmp->parent)
    ((int *)tmp->data)[2] += size;

  return 0;
}

/*
  db_detach:

  Removes child child_num from parent, stores the pointer in entry and adjusts
  sizes.

  Returns:
   < 0 on error
     0 on success
*/
int db_detach (tree_node_t *parent, int child_num, tree_node_t **entry) {
  int size;
  tree_node_t *tmp;

  if (entry == NULL) return -1;
  if (child_num >= parent->num_children) return -1;

  *entry = parent->children[child_num];

  parent->num_children -= 1;

  if (child_num != parent->num_children)
    memcpy (&parent->children[child_num], &parent->children[child_num + 1],
	    parent->num_children - child_num);

  parent->children = realloc (parent->children, parent->num_children *
			      sizeof(tree_node_t *));

  size = db_size_tree (*entry);

  for (tmp = parent ; tmp ; tmp = tmp->parent)
    ((int *)tmp->data)[2] -= size;

  (*entry)->parent = NULL;

  return 0;
}
