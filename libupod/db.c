/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 db.c
 *
 *   Routines for reading/writing the iPod's databases.
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

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/uio.h>

#ifndef S_SPLINT_S
#include <unistd.h>
#endif

#include <fcntl.h>

#include <errno.h>

u_int32_t string_to_int (unsigned char *string) {
  return string[0] << 24 | string[1] << 16 | string[2] << 8 | string[3];
}

/*
  db_size_tree:

    Internal utility function for acumulating the total size of
  the database below an entry.
*/
static int db_size_tree (tree_node_t *ptr) {
  int i, size = ptr->data_size;

  for (i = 0 ; i < ptr->num_children ; i++)
    size += db_size_tree (ptr->children[i]);

  return size;
}

/*
  db_free_tree:

  Function to recursively free a tree.
*/
void db_free_tree (tree_node_t *ptr) {
  if (ptr == NULL)
    return;

  if (ptr->children != NULL) {
    while (--ptr->num_children > -1)
      db_free_tree(ptr->children[ptr->num_children]);

    free(ptr->children);
    ptr->children = NULL;
  }

  free(ptr->data);
  ptr->data = NULL;
  free (ptr);
}

/**
  db_free:

  Frees the memory that is allocated to an itunesdb

  Arguments:
   ipoddb_t *itunesdb - opened itunesdb

  Returns:
   nothing, void function
**/
void db_free (ipoddb_t *itunesdb) {
  if (itunesdb == NULL)
    return;

  db_free_tree(itunesdb->tree_root);

  itunesdb->tree_root = NULL;
  free (itunesdb->path);
  itunesdb->path = NULL;
}

static int dohm_contains_string (struct db_dohm *dohm_data) {
  if (dohm_data->type != 0x64 &&
      dohm_data->type != 0x33 &&
      dohm_data->type != 0x34)
    return 1;

  return 0;
}
/*
  db_build_tree:

  Internal function. 

  Purpose is to build up the iTunesDB tree from a buffer.
*/
static tree_node_t *db_build_tree (ipoddb_t *ipod_db, size_t *bytes_read,
				   /*@null@*/tree_node_t *parent, char **buffer) {
  tree_node_t *tnode_0;
  int *iptr = (int *)*buffer;

  int current_bytes_read = *bytes_read;
  int entry_size, cell_size, copy_size;

  /* swap the entry label, cell size and entry size */
  bswap_block(*buffer, 4, 3);

  if ((iptr[0] & 0x0000686d) != 0x0000686d) {
    db_log (ipod_db, -1, "db_load: Database has a major error or is unsupported\n");

    return NULL;
  }

  tnode_0 = calloc (1, sizeof(tree_node_t));

  if (tnode_0 == NULL) {
    perror("db_build_tree|calloc");
    exit(EXIT_FAILURE);
  }

  tnode_0->parent = parent;


  entry_size = iptr[2];
  cell_size  = iptr[1];

  bswap_block (&((*buffer)[0x0c]), 4, cell_size/4 - 3);

  copy_size = cell_size;

  if ( (iptr[0] == DOHM) && (cell_size != entry_size) ) {
    struct db_dohm *dohm_data = (struct db_dohm *)*buffer;
    
    bswap_block (&((*buffer)[0x18]), 4, 1);

    /* A dohm cell can hold a string, data, or a sub-tree. Process data/string
       dohm cells in a different way than those that have subtrees. */
    if ((iptr[6] & 0x0000686d) != 0x0000686d) {
      copy_size = entry_size;

      if (dohm_contains_string(dohm_data) == 0) {
	/* Read a data dohm */
	copy_size = entry_size;
	bswap_block (&((*buffer)[0x1c]), 4, entry_size/4 - 7);
      } else {
	/* Read a string dohm */
	if (iptr[9] == 0)
	  tnode_0->string_header_size = 16;
	else
	  /* ArtworkDB string dohms are smaller by four bytes than the comparable
	     iTunesdb string dohm. */
	  tnode_0->string_header_size = 12;

	bswap_block (&((*buffer)[0x1c]), 4, tnode_0->string_header_size/4 - 1);

	/* Swap UTF-16 strings */
	if (tnode_0->string_header_size == 16) {
	  struct string_header_16 *string_header = (struct string_header_16 *)&((*buffer)[0x18]);
	  if (string_header->format != 1)
	    bswap_block (&((*buffer)[0x28]), 2, string_header->string_length/2);
	} else {
	  struct string_header_12 *string_header = (struct string_header_12 *)&((*buffer)[0x18]);
	  if (string_header->format != 1)
	    bswap_block (&((*buffer)[0x24]), 2, string_header->string_length/2);
	}
      }
    } else
      bswap_block (&((*buffer)[0x18]), 4, 1);

  } else if (iptr[0] == TIHM) {
    struct db_tihm *tihm_data = (struct db_tihm *)(*buffer);
    
    ipod_db->last_entry = ((ipod_db->last_entry < tihm_data->identifier)
			  ? tihm_data->identifier
			  : ipod_db->last_entry);
  } else if (iptr[0] == IIHM) {
    struct db_iihm *iihm_data = (struct db_iihm *)(*buffer);

    ipod_db->last_entry = ((ipod_db->last_entry < iihm_data->identifier)
			  ? iihm_data->identifier
			  : ipod_db->last_entry);
  }
  

  tnode_0->num_children = 0;
  tnode_0->data_size = copy_size;
  tnode_0->data = calloc (copy_size, 1);

  if (tnode_0->data == NULL) {
    perror("db_build_tree|calloc");
    exit (EXIT_FAILURE);
  }

  memmove (tnode_0->data, *buffer, copy_size);

  /*  db_log (ipod_db, 0, "Loaded tree node: type: %s, size: %08x\n", iptr, copy_size); */

  *buffer     += copy_size;
  *bytes_read += copy_size;

  iptr = (int *)tnode_0->data;

  if (copy_size == entry_size ||
      iptr[0] == ALHM ||
      iptr[0] == FLHM ||
      iptr[0] == ILHM ||
      iptr[0] == PLHM ||
      iptr[0] == TLHM)
    goto dbbt_done;

  tnode_0->children = calloc(1, sizeof(tree_node_t *));
  
  if (tnode_0->children == NULL) {
    perror("db_build_tree|calloc");
    exit (EXIT_FAILURE);
  }

  while (*bytes_read - current_bytes_read < entry_size) {
    tnode_0->children = realloc(tnode_0->children, ++(tnode_0->num_children) *
				sizeof(tree_node_t *));
    
    if (tnode_0->children == NULL) {
      perror("db_build_tree|realloc");
      exit (EXIT_FAILURE);
    }

    tnode_0->children[tnode_0->num_children-1] = db_build_tree(ipod_db, bytes_read, tnode_0, buffer);
  }

 dbbt_done:
  return tnode_0;
}

/**
  db_load:

  Function that loads an itunesdb from a file. If volume is NULL then
  the database is loaded from a unix file described by path.

  Arguments:
   ipoddb_t *itunesdb - pointer to where the itunesdb should go.
   HFSPlus    *volume   - the HFSPlus volume to load from
   char       *path     - the path (including partition name) to where the database is

  Returns:
   < 0 on error
   bytes read on success
**/
int db_load (ipoddb_t *ipod_db, char *path, int flags) {
  int iPod_DB_fd;
  char *buffer;
  int ibuffer[3];
  int ret;
  struct stat statinfo;

  int *tmp;

  size_t bytes_read  = 0;

  if (path == NULL || strlen(path) == 0 || ipod_db == NULL)
    return -EINVAL;

  db_log (ipod_db, 0, "db_load: entering...\n");

  if (stat(path, &statinfo) < 0) {
    db_log (ipod_db, errno, "db_load|stat: %s\n", strerror(errno));

    return -1;
  }

  if (!S_ISREG(statinfo.st_mode)) {
    db_log (ipod_db, -1, "db_load: %s, not a regular file\n", path);

    return -1;
  }

  db_log (ipod_db, 0, "db_load: Attempting to read an iPod database: %s\n", path);

  if ((iPod_DB_fd = open (path, O_RDONLY)) < 0) {
    db_log (ipod_db, errno, "db_load|open: %s\n", strerror(errno));

    return -errno;
  }

  /* read in the size of the database */
  read (iPod_DB_fd, ibuffer, 12);

  bswap_block((char *)ibuffer, 4, 3);

  if (ibuffer[0] == DBHM) {
    db_log (ipod_db, 0, "db_load: Reading an iTunesDB.\n");
    ipod_db->type = 0;
  } else if (ibuffer[0] == DFHM) {
    db_log (ipod_db, 0, "db_load: Reading a photo or artwork database.\n");
    ipod_db->type = 1;
  } else {
    db_log (ipod_db, -1, "db_load: %s is not a valid database. Exiting.\n", path);
    close (iPod_DB_fd);

    return -1;
  }
  
  buffer = (char *)calloc(ibuffer[2], 1);
  if (buffer == NULL) {
    db_log (ipod_db, errno, "db_load: Could not allocate memory\n");
    close (iPod_DB_fd);

    return -errno;
  }

  /* keep track of where buffer starts */
  tmp = (int *)buffer;

  if (ibuffer[2] != statinfo.st_size)
    db_log (ipod_db, -1, "db_load: File size does not match database size! Continuing anyway.\n");

  /* read in the rest of the database */
  if ((ret = read (iPod_DB_fd, buffer + 12, ibuffer[2] - 12)) <
      (ibuffer[2] - 12)) {
    db_log (ipod_db, errno, "db_load: Short read: %i bytes wanted, %i read\n", ibuffer[2],
	    ret);
    
    free(buffer);
    close(iPod_DB_fd);
    return -1;
  }

  bswap_block((char *)ibuffer, 4, 3);
  memmove (buffer, ibuffer, 12);
  
  close (iPod_DB_fd);

  /* Load the database into a tree structure. */
  ipod_db->tree_root = db_build_tree (ipod_db, &bytes_read, NULL, &buffer);

  ipod_db->flags = flags;
  ipod_db->path  = strdup (path);

  /*
    This pointer points to the space allocated for reading from the file.
    Free it now as it is no longer useful.
  */
  free(tmp);

  if (ipod_db->type == 0) {
    /*
      Remove index dohms from the main playlist as they will be invalid
      after any change to the database. The indices will be generated
      when the database is written out to a file.
    */
    db_playlist_strip_indices (ipod_db);
  }

  db_log (ipod_db, 0, "db_load: complete. loaded %i B\n", bytes_read);

  return bytes_read;
}

static int db_write_tree (int fd, tree_node_t *entry) {
  int ret = 0;
  int i, swap;
  struct db_dohm *dohm_data;

#if BYTE_ORDER == BIG_ENDIAN
  dohm_data = (struct db_dohm *) entry->data;

  if (dohm_data->dohm == DOHM) {
    if (entry->string_header_size == 16) {
      struct string_header_16 *string_header;

      string_header = (struct string_header_16 *)&entry->data[0x18];

      swap = 10;

      if (string_header->format != 1)
	bswap_block (&(entry->data[0x28]), 2, string_header->string_length/2);
    } else if (entry->string_header_size == 12) {
      struct string_header_12 *string_header;

      string_header = (struct string_header_12 *)&entry->data[0x18];

      swap = 9;
      
      if (string_header->format != 1)
	bswap_block (&(entry->data[0x24]), 2, string_header->string_length/2);
    } else
      swap = entry->data_size/4;

  } else
    swap = ((int *)entry->data)[1]/4;

  bswap_block(entry->data, 4, swap);
#endif

  ret += write (fd, entry->data, entry->data_size);


#if BYTE_ORDER == BIG_ENDIAN
  bswap_block(entry->data, 4, swap);

  if (dohm_data->dohm == DOHM && (dohm_data->type & 0x01000000) == 0) {
    if (entry->string_header_size == 16) {
      struct string_header_16 *string_header;
      string_header = (struct string_header_16 *)&entry->data[0x18];

      if (string_header->format != 1)
	bswap_block (&(entry->data[0x28]), 2, string_header->string_length/2);
    } else if (entry->string_header_size == 12) {
      struct string_header_12 *string_header;
      string_header = (struct string_header_12 *)&entry->data[0x18];

      if (string_header->format != 1)
	bswap_block (&(entry->data[0x24]), 2, string_header->string_length/2);
    }
  }
#endif

  for (i = 0 ; i < entry->num_children ; i++)
    ret += db_write_tree (fd, entry->children[i]);

  return ret;
}

/*
  db_write:

  Write the ipod_db out to a file.
*/
int db_write (ipoddb_t ipod_db, char *path) {
  int fd;

  int ret;
  int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  if (ipod_db.tree_root == NULL || path == NULL)
    return -EINVAL;

  db_log (&ipod_db, 0, "db_write: entering...\n");

  if ((fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, perms)) < 0) {
    db_log (&ipod_db, -errno, "db_write: error writing %s, %s\n", path, strerror (errno));

    return -errno;
  }

  if (ipod_db.type == 0)
    db_playlist_add_indices (&ipod_db);

  ret = db_write_tree (fd, ipod_db.tree_root);

  if (ipod_db.type == 0)
    db_playlist_strip_indices (&ipod_db);
  
  close (fd);

  db_log (&ipod_db, 0, "db_write: complete. wrote %i B\n", ret);
  
  return ret;
}

int db_attach_at (tree_node_t *parent, tree_node_t *new_child, int index) {
  int size, i;
  tree_node_t *tmp;

  if ((parent == NULL) || (new_child == NULL))
    return -EINVAL;

  if (index >= (parent->num_children + 1))
    return -1;

  /* allocate memory for the new child pointer */
  if (parent->num_children++ == 0)
    parent->children = calloc (1, sizeof(tree_node_t *));
  else
    parent->children = realloc (parent->children, parent->num_children * 
				sizeof (tree_node_t *));

  /* add the new child node onto this node and set its parent
   pointer accordingly */
  new_child->parent = parent;

  if (index < (parent->num_children - 1))
    for (i = parent->num_children - 1 ; i > index ; i--)
      parent->children[i] = parent->children[i-1];

  parent->children[index] = new_child;

  /* adjust tree sizes */
  size = db_size_tree (new_child);

  for (tmp = parent ; tmp ; tmp = tmp->parent)
    ((int *)tmp->data)[2] += size;

  return 0;
}

int db_attach (tree_node_t *parent, tree_node_t *new_child) {
  return db_attach_at (parent, new_child, parent->num_children);
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
  int i;

  if (entry == NULL) return -1;
  if (child_num >= parent->num_children) return -1;

  *entry = parent->children[child_num];
  (*entry)->parent = NULL;

  parent->num_children -= 1;

  if (child_num != parent->num_children)
    for (i = child_num ; i < parent->num_children ; i++)
      parent->children[i] = parent->children[i+1];

  parent->children = realloc (parent->children, parent->num_children *
			      sizeof(tree_node_t *));

  size = db_size_tree (*entry);

  for (tmp = parent ; tmp ; tmp = tmp->parent)
    ((int *)tmp->data)[2] -= size;

  (*entry)->parent = NULL;

  return 0;
}

int db_node_allocate (tree_node_t **entry, unsigned long type, size_t size, int subtree) {
  struct db_generic *data;
  if (entry == NULL)
    return -EINVAL;

  *entry = (tree_node_t *)calloc(1, sizeof(tree_node_t));
  if (*entry == NULL) {
    perror ("db_node_allocate|calloc");

    exit (EXIT_FAILURE);
  }

  (*entry)->data_size = size;
  (*entry)->data = calloc ((*entry)->data_size, 1);
  if ((*entry)->data == NULL) {
    perror ("db_node_allocate|calloc");

    exit (EXIT_FAILURE);
  }

  data = (struct db_generic *)(*entry)->data;
  
  data->type         = type;
  data->cell_size    = size;

  /* Tree nodes do not always use this integer for a subtree-size. */
  data->subtree_size = subtree;

  return 0;
}
