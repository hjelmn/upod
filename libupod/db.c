/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a-10 db.c
 *
 *   Routines for reading/writing the iPod's iTunesDB.
 *
 *   Quick Note: I use cell, record, and entry all to mean the same thing.
 *    Eventually I will clean this up and try to be a little more consistant
 *    with my language, so keep that in mind while reading my code.
 *
 *   Changes:
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "upodi.h"

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <errno.h>

int ipod_debug = 1;

/*
  db_size_tree:

    Internal utility function for acumulating the total size of
  the database below an entry.
*/
static int db_size_tree (struct tree_node *ptr) {
  int i, size = ptr->size;

  for (i = 0 ; i < ptr->num_children ; i++)
    size += db_size_tree (ptr->children[i]);

  return size;
}

/*
  db_free_tree:

  Function to recursively free a tree.
*/
void db_free_tree (struct tree_node *ptr) {
  if (ptr == NULL) return;

  if (!strstr(ptr->data, "dohm") && ptr->num_children) {
    while (--ptr->num_children > -1)
      db_free_tree(ptr->children[ptr->num_children]);

    free(ptr->children);
  }

  free(ptr->data);
  free(ptr);
}

/*
  db_free:

  Frees the memory that is allocated to the iTunesDB within ipod.

  Returns:
   nothing
*/
void db_free (ipod_t *ipod) {
  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return;

  db_free_tree(ipod->iTunesDB.tree_root);
}

/*
  db_build_tree:

  Internal function. 

  Purpose is to build up the iTunesDB tree from a buffer.
*/
static struct tree_node *db_build_tree (size_t *bytes_read,
					struct tree_node *parent,
					char **buffer) {
  struct tree_node *tnode_0;
  int *iptr = (int *)*buffer;
  int i;

  int current_bytes_read = *bytes_read;
  int entry_size, cell_size, copy_size;

  tnode_0 = malloc (sizeof(struct tree_node));

  if (tnode_0 == NULL) {
    perror("db_build_tree|malloc");
    exit(1);
  }

  memset (tnode_0, 0, sizeof(struct tree_node));

  tnode_0->parent = parent;

  bswap_block(*buffer, sizeof(u_int32_t), 3);
  entry_size = iptr[2];
  cell_size  = iptr[1];

  if (strstr(*buffer, "dohm")) {
    /* swap only the header and not the data for this entry */
    bswap_block(&((*buffer)[0xc]), 4, 7);

    copy_size = entry_size;
  } else {
    bswap_block(&(*buffer)[0xc], sizeof(u_int32_t), cell_size/4 - 3);

    copy_size = cell_size;
  }

  tnode_0->num_children = 0;
  tnode_0->size = copy_size;
  tnode_0->data = malloc (copy_size);
  memcpy (tnode_0->data, *buffer, copy_size);
  
  *buffer     += copy_size;
  *bytes_read += copy_size;

  /* becuase the tlhm structures second field is not a total size,
     do not do anything more with it */
  if (cell_size == entry_size ||
      strstr(tnode_0->data, "tlhm") ||
      strstr(tnode_0->data, "dohm") )
    goto dbbt_done;

  tnode_0->children = malloc(sizeof(struct tree_node *));

  if (tnode_0->children == NULL) {
    perror("db_build_tree|malloc");
    exit(1);
  }

  while (*bytes_read - current_bytes_read < entry_size) {
    tnode_0->children = realloc(tnode_0->children, 
				++(tnode_0->num_children) *
				sizeof(struct tree_node *));
    
    if (tnode_0->children == NULL) {
      perror("db_build_tree|realloc");
      fprintf(stderr, "requested size was 0x%08x B\n",
	      (tnode_0->num_children) * 4);
      exit(1);
    }

    tnode_0->children[tnode_0->num_children-1] = db_build_tree(bytes_read,
							       tnode_0,
							       buffer);
  }

 dbbt_done:
  return tnode_0;
}

/*
  db_load:

  Function that loads an itunesdb from a file.

   In the future this may also handle loading the database from
  a file through the hfsplus utilities.

  Returns:
   < 0 for error
     0 on success
*/
int db_load (ipod_t *ipod, char *path) {
  int iTunesDB;
  char *buffer;
  int ibuffer[3];
  int ret;

  int *tmp;

  size_t bytes_read  = 0;

  iTunesDB = open(path, O_RDONLY);
  if (iTunesDB < 0) UPOD_ERROR(errno, "Error opening db\n");

  read(iTunesDB, ibuffer, 12);
  bswap_block((char *)ibuffer, 4, 3);

  buffer = (char *)malloc(ibuffer[2]);
  if (buffer == NULL) UPOD_ERROR(errno, "Could not allocate memory\n");

  /* keep track of where buffer starts */
  tmp = (int *)buffer;

  if ((ret = read(iTunesDB, buffer + 12, ibuffer[2] - 12)) < (ibuffer[2] - 12)) {
    UPOD_ERROR(errno, "Short read: %i bytes wanted, %i read\n", ibuffer[2], ret);
    close(iTunesDB);
    free(buffer);
    return -1;
  }
  bswap_block((char *)ibuffer, 4, 3);
  memcpy (buffer, ibuffer, 12);
  close(iTunesDB);

  ipod->iTunesDB.tree_root = db_build_tree(&bytes_read, NULL, &buffer);

  free(tmp);

  return bytes_read;
}

/*
  db_write_tree:

  Internal function.

  Recursivly parse the database tree and write it to a file.
*/
static int db_write_tree (int fd, struct tree_node *entry) {
  static int ret;
  int i, swap;

#if BYTE_ORDER == BIG_ENDIAN
  if (strstr(entry->data, "dohm"))
    swap = 10;
  else
    swap = ((int *)entry->data)[1]/4;

  bswap_block(entry->data, 4, swap);
#endif

  ret += write(fd, entry->data, entry->size);

#if BYTE_ORDER == BIG_ENDIAN
  bswap_block(entry->data, 4, swap);
#endif

  for (i = 0 ; i < entry->num_children ; i++)
    db_write_tree(fd, entry->children[i]);

  return ret;
}

/*
  db_write:

    Writes the database the the file specified by path.

  Returns:
   < 0 on error
     0 on success
*/
int db_write (ipod_t ipod, char *path) {
  int fd, ret;
  int perms;

  perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  if (ipod.iTunesDB.tree_root == NULL) return -1;

  fd = open(path, O_WRONLY | O_CREAT, perms);

  if (fd < 0) {
    perror("db_write");
    return errno;
  }

  ret = db_write_tree (fd, ipod.iTunesDB.tree_root);

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
int db_hide (ipod_t *ipod, u_int32_t tihm_num) {
  return db_remove_from_playlist (ipod, 0, tihm_num);
}

/*
  db_unhide:

  Adds a reference to song tihm_num to the master playlist.

  Returns:
   < 0 on error
     0 on success
*/
int db_unhide (ipod_t *ipod, u_int32_t tihm_num) {
  return db_add_to_playlist (ipod, 0, tihm_num);
}

/*
  db_remove:

   Deletes an entry from the song list, then deletes the referance (if any)
  from the master playlist.

  XXX -- TODO -- We may want to parse all playlists and remove the references
  from them as well.

  Returns:
   < 0 if any error occured.
     0 if successfull.
*/
int db_remove (ipod_t *ipod, u_int32_t tihm_num) {
  struct tree_node *parent, *entry;
  struct db_tlhm *tlhm;
  int size, entry_num;

  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return -1;

  entry_num = db_tihm_retrieve (ipod->iTunesDB, &entry, &parent, tihm_num);

  if (entry_num < 0) {
    if (ipod_debug)
      fprintf(stderr, "db_remove %i: no song found\n", tihm_num);

    return entry_num;
  }

  tlhm = (struct db_tlhm *)parent->children[0]->data;
  tlhm->num_tihm -= 1;

  /* remove the entry */
  db_detach (parent, entry_num, &entry);
  db_free_tree (entry);
 
  /* remove from master playlist */
  db_hide (ipod, tihm_num);

  return 0;
}

/*
  db_add:

   Adds a song to the song list, then adds a reference to that song to the
  master playlist.

  Returns:
   < 0 on error
     0 on success
*/
int db_add (ipod_t *ipod, char *filename, char *path) {
  struct tree_node **master, *entry, *root;
  struct db_tlhm *tlhm;
  int tihm_num, prev_tihm_num, size;

  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return -1;

  root = ipod->iTunesDB.tree_root;

  /* The song list resides in the first dshm entry of the iTunesDB */
  for (master = &(root->children[0]) ;
       !strstr((*master)->data, "dshm") ; master++);

  tlhm = (struct db_tlhm *)(*master)->children[0]->data;
  tlhm->num_tihm += 1;

  entry = (struct tree_node *)malloc(sizeof(struct tree_node));
  entry->parent = *master;
  
  tihm_num = db_tihm_create (entry, filename, path);
  db_attach (*master, entry);
  
  db_unhide(ipod, tihm_num);

  return 0;
}

/*
  db_modify_eq:

  Modifies (or adds) an equilizer setting of(to) a song entry.

  Returns:
   < 0 if an error occured
     0 if successful
*/
int db_modify_eq (ipod_t *ipod, u_int32_t tihm_num, int eq) {
  struct tree_node *entry, *dohm;
  struct db_tihm *tihm;

  int size;

  if (ipod == NULL) return -1;

  if (db_tihm_retrieve (ipod->iTunesDB, &entry, NULL, tihm_num) < 0) {
    if (ipod_debug)
      fprintf(stderr, "db_song_modify_eq %i: no song found\n", tihm_num);

    return -2;
  }

  /* see if an equilizer entry already exists */
  dohm = db_dohm_search (entry, 0x7);

  if (dohm == NULL) {
    dohm = malloc (sizeof(struct tree_node));

    if (dohm == NULL) {
      perror ("db_song_modify_eq");

      return -1;
    }

    memset (dohm, 0, sizeof(struct tree_node));

    db_attach (entry, dohm);

    tihm = (struct db_tihm *)entry->data;
    tihm->num_dohm ++;
  }

  if (db_dohm_create_eq (dohm, eq) < 0)
    return -1;

  return 0;
}

/*
  db_modify_volume_adjustment:

    Mofifies the volume adjustment of a song entry. Acceptible values
  are between -100 and +100

  Returns:
   < 0 on error
     0 on success
*/
int db_modify_volume_adjustment (ipod_t *ipod, u_int32_t tihm_num,
				 int volume_adjustment) {
  struct tree_node *entry;
  struct db_tihm *tihm;

  if (ipod == NULL) return -1;

  /* make sure the volume adjustment value is valid */
  if (volume_adjustment > 100 || volume_adjustment < -100) return -2;

  db_tihm_retrieve (ipod->iTunesDB, &entry, NULL, tihm_num);

  if (entry == NULL) {
    if (ipod_debug)
      fprintf(stderr, "db_song_modify_volume_adjustment %i: no song found\n",
	      tihm_num);

    return -2;
  }

  tihm = (struct db_tihm *)entry->data;

  tihm->volume_adjustment = volume_adjustment;

  return 0;
}

/*
  db_modify_start_stop_time:

    Modifies the start and stop time of a song entry. Arguments are in secs even
  though they are stored in microsecs in the iTunesDB.

  Returns:
   < 0 if an error occured
     0 if successful
*/
int db_modify_start_stop_time (ipod_t *ipod, u_int32_t tihm_num, int start_time,
			       int stop_time) {
  struct tree_node *entry;
  struct db_tihm *tihm;

  if (ipod == NULL) return -1;

  db_tihm_retrieve (ipod->iTunesDB, &entry, NULL, tihm_num);

  if (entry == NULL) {
    if (ipod_debug)
      fprintf(stderr, "db_song_modify_volume_adjustment %i: no song found\n",
	      tihm_num);

    return -2;
  }

  tihm = (struct db_tihm *)entry->data;

  stop_time *= 1000;
  start_time *= 1000;

  if (start_time > stop_time) return -3;

  /* make sure that the requested values are within the duration of the song */
  if (stop_time > tihm->duration || start_time > tihm->duration)
    return -4;

  tihm->start_time = start_time;
  tihm->stop_time  = stop_time;

  return 0;
}

/*
  db_song_list:

  Returns a linked list of the songs that are currently in the song list
  of the iTunesDB.

  Returns:
   NULL if any error occured.
   ptr  if successfull.
*/
GList *db_song_list (ipod_t ipod) {
  GList **ptr, *head;
  struct tree_node **master, *root, *entry;
  int i;
  
  root = ipod.iTunesDB.tree_root;

  head = (GList *) malloc (sizeof(GList));
  memset(head, 0, sizeof(GList));
  ptr  = &(head->next);
  
  master = root->children;

  for ( i = 0 ; i < root->num_children ; i++)
    if (strstr(master[i]->data, "dshm"))
      break;

  if (i == root->num_children) {
    free(head);
    return NULL;
  }

  master = &master[i];

  for (i = 0 ; i < (*master)->num_children ; i++) {
    entry = (*master)->children[i];

    /* only add tihm entries to the song list */
    if (!strstr(entry->data, "tihm"))
      continue;

    *ptr  = (GList *) malloc (sizeof(GList));

    (*ptr)->data = (void *) db_tihm_fill (entry);
    (*ptr)->next = NULL;

    ptr = &((*ptr)->next);
  }

  return head;
}

/*
  db_song_list_free:

  Frees the memory that has been allocated to a song list. I strongly
  recomend you call this to clean up at the end of execution.

  Returns:
   nothing, void function
*/
void db_song_list_free (GList *head) {
  GList *tmp;
  while (head) {
    tmp = head->next;

    tihm_free ((tihm_t *)head->data);

    free(head);
    head = tmp;
  }
}

int db_attach (struct tree_node *parent, struct tree_node *new_child) {
  int size;
  struct tree_node *tmp;

  if (parent == NULL || new_child == NULL) return -1;

  /* allocate memory */
  if (parent->num_children++ == 0)
    parent->children = malloc (sizeof (struct tree_node *));
  else
    parent->children = realloc (parent->children, parent->num_children * 
				sizeof (struct tree_node *));

  /* adjust pointers */
  new_child->parent = parent;
  parent->children[parent->num_children - 1] = new_child;

  /* adjust size */
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
int db_detach (struct tree_node *parent, int child_num, struct tree_node **entry) {
  int size;
  struct tree_node *tmp;

  if (entry == NULL) return -1;
  if (child_num >= parent->num_children) return -1;

  *entry = parent->children[child_num];

  parent->num_children -= 1;

  if (child_num != parent->num_children)
    memcpy (&parent->children[child_num], &parent->children[child_num + 1],
	    parent->num_children - child_num);

  parent->children = realloc (parent->children, parent->num_children *
			      sizeof(struct tree_node *));

  size = db_size_tree (*entry);

  for (tmp = parent ; tmp ; tmp = tmp->parent)
    ((int *)tmp->data)[2] -= size;

  (*entry)->parent = NULL;

  return 0;
}
