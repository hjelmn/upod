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

static int db_tree_size (struct tree_node *ptr) {
  int i, size = ptr->size;
  for (i = 0 ; i < ptr->num_children ; i++)
    size += db_tree_size (ptr->children[i]);

  return size;
}

static void db_free_tree (struct tree_node *ptr) {
  if (ptr == NULL) return;

  if (!strstr(ptr->data, "dohm") && ptr->num_children) {
    while (--ptr->num_children > -1)
      db_free_tree(ptr->children[ptr->num_children]);

    free(ptr->children);
  }

  free(ptr->data);
  free(ptr);
}

void db_free (ipod_t *ipod) {
  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return;

  db_free_tree(ipod->iTunesDB.tree_root);
}

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

  Removes and entry from the master playlist.

  Returns:
    < 0 if an error occurs.
      0 if successfull.
*/
int db_hide (ipod_t *ipod, u_int32_t tihm_num) {
  struct tree_node **master, *entry, *root;
  struct db_pyhm *pyhm;
  int entry_num, size;

  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return -1;

  root = ipod->iTunesDB.tree_root;
  
  /* master playlist resides in the last dshm entry */
  for (master = &root->children[root->num_children-1] ;
       !strstr((*master)->data, "dshm") ; master--);

  for (master = &((*master)->children[0]) ;
       !strstr((*master)->data, "pyhm") ; master++);

  pyhm = (struct db_pyhm *)(*master)->data;
  pyhm->num_pihm -= 2;

  entry_num = db_pihm_search (*master, tihm_num);

  if (entry_num < 0) {
    if (ipod_debug)
      fprintf(stderr, "db_hide %i: no song found\n", tihm_num);

    return 2;
  }

  entry = (*master)->children[entry_num];

  size = entry->size + (*master)->children[entry_num + 1]->size;

  printf("removing size %i\n", size);

  /* remove the entry */
  db_free_tree (entry);
  db_free_tree ((*master)->children[entry_num + 1]);

  (*master)->num_children -= 2;

  /* move remaining entries up */
  if (entry_num != (*master)->num_children)
    memcpy (&((*master)->children[entry_num]),
	    &((*master)->children[entry_num + 2]),
	    ((*master)->num_children - entry_num)
	    * sizeof (struct tree_node *));

  (*master)->children = realloc ((*master)->children,
				 (*master)->num_children
				 * sizeof(struct tree_node *));

  /* adjust sizes */
  for (entry = *master ; entry ; entry = entry->parent)
    ((int *)entry->data)[2] -= size;

  return 0;
}

/*
  db_unhide:

  Adds an entry to the master playlist.

  Returns:
   < 0 if any error occured.
     0 if successfull.
*/
int db_unhide (ipod_t *ipod, u_int32_t tihm_num) {
  struct tree_node **master, *entry, *root;
  struct db_pyhm *pyhm;
  int entry_num, size, junk = 0, i;

  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return -1;

  root = ipod->iTunesDB.tree_root;
  
  /* the master playlist is stored in the last dshm entry in
     the iTunesDB */
  for (master = &root->children[root->num_children-1] ;
       !strstr((*master)->data, "dshm") ; master--);

  /* within this dshm entry is the master playlist, it is the first pyhm
     entry there */
  for (master = &((*master)->children[0]) ;
       !strstr((*master)->data, "pyhm") ; master++);

  /* increment the total number of songs contained in the playlist */
  pyhm = (struct db_pyhm *)((*master)->data);
  pyhm->num_pihm += 2; /* actually, total number of pihms and dohms (i think) */

  (*master)->num_children += 2;

  (*master)->children = realloc ((*master)->children,
				 (*master)->num_children * sizeof(struct tree_node *));

  if ((*master)->children == NULL) {
    perror("db_unhide|realloc");
    exit(1);
  }

  junk = tihm_num;

  entry = (struct tree_node *)malloc(sizeof(struct tree_node));

  if (entry == NULL) {
    perror("db_unhide|malloc");
    exit(1);
  }

  entry->parent = *master;
  db_pihm_create(entry, tihm_num, junk);
  (*master)->children[(*master)->num_children - 2] = entry;

  entry = (struct tree_node *)malloc(sizeof(struct tree_node));

  if (entry == NULL) {
    perror("db_unhide|malloc");
    exit(1);
  }

  entry->parent = *master;
  /* every pihm entry is followed by a dohm entry of size 0x2C.
     I have no idea what the data contained within it means 
     (therefore i call it a generic entry).
  */
  db_dohm_create_generic (entry, 0x2c, junk);

  (*master)->children[(*master)->num_children - 1] = entry;

  size = entry->size + (*master)->children[(*master)->num_children-2]->size;

  /* of course, the sizes of all the parent records must be increased */
  for (entry = *master ; entry ; entry = entry->parent)
    ((int *)entry->data)[2] += size;

  return 0;
}

/*
  db_remove:

  Removes an entry from the song list, then removes the song
  from the master playlist.

  Returns:
   < 0 if any error occured.
     0 if successfull.
*/
int db_remove (ipod_t *ipod, u_int32_t tihm_num) {
  struct tree_node **master, *entry, *root;
  struct db_tlhm *tlhm;
  int size, entry_num;

  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return -1;

  root = ipod->iTunesDB.tree_root;

  /* the song list resides in the first dshm entry of the iTunesDB */
  for (master = &(root->children[0]) ;
       !strstr((*master)->data, "dshm") ; master++);

  tlhm = (struct db_tlhm *)(*master)->children[0]->data;
  tlhm->num_tihm -= 1;

  entry_num = db_tihm_search (*master, tihm_num);

  if (entry_num < 0) {
    if (ipod_debug)
      fprintf(stderr, "db_remove %i: no song found\n", tihm_num);

    return 2;
  }

  entry = (*master)->children[entry_num];

  size = db_tree_size (entry);

  printf("removing size %i\n", size);

  /* remove the entry */
  db_free_tree (entry);
  
  (*master)->num_children--;

  if (entry_num != (*master)->num_children)
    memcpy (&((*master)->children[entry_num]),
	    &((*master)->children[entry_num + 1]),
	    ((*master)->num_children - entry_num)
	    * sizeof (struct tree_node *));

  (*master)->children = realloc ((*master)->children,
				 (*master)->num_children
				 * sizeof(struct tree_node *));

  /* adjust sizes */
  for (entry = *master ; entry ; entry = entry->parent)
    ((int *)entry->data)[2] -= size;

  /* remove from master playlist */
  db_hide (ipod, tihm_num);

  return 0;
}

/*
  db_add:

  Adds a song the the song list, then adds that song to the master playlist.

  Returns:
   < 0 if any error occurs.
     0 if successfull.
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

  (*master)->num_children++;
  (*master)->children = realloc ((*master)->children,
				 (*master)->num_children
				 * sizeof(struct tree_node *));

  (*master)->children[(*master)->num_children - 1] = entry;

  size = db_tree_size (entry);

  for (entry = *master ; entry ; entry = entry->parent)
    ((int *)entry->data)[2] += size;

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
  struct tree_node **master, *entry, *root, *dohm;
  struct db_tihm *tihm;
  int size, entry_num;

  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return -1;

  root = ipod->iTunesDB.tree_root;

  /* the song list resides in the first dshm entry of the iTunesDB */
  for (master = &(root->children[0]) ;
       !strstr((*master)->data, "dshm") ; master++);

  entry_num = db_tihm_search (*master, tihm_num);

  if (entry_num < 0) {
    if (ipod_debug)
      fprintf(stderr, "db_song_modify_eq %i: no song found\n", tihm_num);

    return -2;
  }

  entry = (*master)->children[entry_num];

  /* see if an equilizer entry already exists */
  dohm = db_dohm_search (entry, 0x7);

  if (dohm == NULL) {
    dohm = malloc (sizeof(struct tree_node));

    if (dohm == NULL) {
      perror ("db_song_modify_eq");

      return -1;
    }

    memset (dohm, 0, sizeof(struct tree_node));

    entry->num_children++;
    entry->children = realloc (entry->children,
			       entry->num_children * sizeof(struct tree_node));
    entry->children[entry->num_children - 1] = entry->children[entry->num_children -2];
    entry->children[entry->num_children - 2] = dohm;

    tihm = (struct db_tihm *)entry->data;
    tihm->num_dohm ++;
  }

  if (db_dohm_create_eq (dohm, eq) < 0)
    return -1;

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
