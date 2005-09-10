/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.1 db.c
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

db_list_t *db_list_first (db_list_t *p) {
  db_list_t *x;

  for (x = p ; x && x->prev ; x = x->prev);

  return x;
}

db_list_t *db_list_prepend (db_list_t *p, void *d) {
  db_list_t *x, *y;

  x = calloc (1, sizeof (db_list_t));
  x->data = d;

  y = db_list_first (p);

  x->next = y;

  if (y)
    y->prev = x;

  return x;
}

db_list_t *db_list_append (db_list_t *p, void *d) {
  db_list_t *x, *y;

  x = calloc (1, sizeof (db_list_t));
  x->data = d;

  for (y = db_list_first (p) ; y && y->next ; y = db_list_next (y));

  x->prev = y;

  if (y) {
    y->next = x;
    return p;
  }

  return x;
}

db_list_t *db_list_next (db_list_t *p) {
  if (p == NULL)
    return p;

  return p->next;
}

void db_list_free (db_list_t *p) {
  db_list_t *tmp;

  p = db_list_first (p);

  while (p) {
    tmp = p->next;
    free (p);
    p = tmp;
  }
}

/*
  db_size_tree:

  Internal utility function for calculating the total size of
  of a sub-tree.
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

  if (ptr->data_isalloced)
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

  if (itunesdb->path != NULL) {
    free (itunesdb->path);
    itunesdb->path = NULL;
  }
}

static int dohm_contains_string (struct db_dohm *dohm_data) {
  if (dohm_data->type != 0x64 &&
      dohm_data->type != 0x33 &&
      dohm_data->type != 0x34)
    return 1;

  return 0;
}

int tree_node_create (tree_node_t **tnode_0, tree_node_t *parent, u_int8_t *data, size_t data_size,
		      int is_alloced) {
  if (tnode_0 == NULL)
    return -EINVAL;

  *tnode_0 = calloc (1, sizeof (tree_node_t));
  if (*tnode_0 == NULL)
    return -errno;

  (*tnode_0)->parent = parent;
  (*tnode_0)->data = data;
  (*tnode_0)->data_size = data_size;
  (*tnode_0)->data_isalloced = is_alloced;

  return 0;
}

int is_db_cell (u_int8_t *x) {
#if BYTE_ORDER==BIG_ENDIAN
  if (memcmp (&x[2], "hm", 2) == 0)
#else
  if (memcmp (x, "mh", 2) == 0)
#endif
    return 1;

  return 0;
}

int is_list_header (u_int8_t *x) {
#if BYTE_ORDER==BIG_ENDIAN
  if (memcmp (&x[1], "lhm", 3) == 0)
#else
  if (memcmp (x, "mhl", 3) == 0)
#endif
    return 1;

  return 0;
}

/*
  db_build_tree (internal):

  Parse an iTunes or Artwork(Photo) database and build the tree structure used by upod.
*/
static int db_build_tree (ipoddb_t *ipod_db, tree_node_t **tnode_0p, size_t *bytes_readp,
			  size_t tree_size, /*@null@*/tree_node_t *parent, char *buffer) {
  tree_node_t *tnode_0;
  size_t bytes_read = *bytes_readp;
  u_int32_t copy_size;

  int is_alloced = 0;
  int has_children = 0;
  u_int8_t *data;
  
  int last_cell = 0;

  struct db_generic *cell = (struct db_generic *)&buffer[bytes_read];
  int ret;

  bswap_block (&buffer[bytes_read], 4, 3);

  if (!is_db_cell (&buffer[bytes_read])) {
    db_log (ipod_db, -1, "db_load: unknown cell type: %c%c%c%c. aborting..\n", buffer[bytes_read],
	    buffer[bytes_read + 1], buffer[bytes_read + 2], buffer[bytes_read + 3]);

    return -1;
  }

  /* swap the cell's data + one extra word for child node check */
  if ((bytes_read + cell->cell_size) >= tree_size) {
    last_cell = 1;

    bswap_block (&buffer[bytes_read + 0x0c], 4, cell->cell_size/4 - 3);
  } else
    bswap_block (&buffer[bytes_read + 0x0c], 4, cell->cell_size/4 - 2);


  copy_size = cell->subtree_size; 

  /* check if the current cell has any children */
  if (!last_cell) {
    if (is_db_cell (&buffer[bytes_read + cell->cell_size])) {
      copy_size = cell->cell_size;

      if (!is_list_header (&buffer[bytes_read]))
	has_children = 1;
    }

    bswap_block (&buffer[bytes_read + cell->cell_size], 4, 1);
  }

  /* load data into cell */
  if (!(ipod_db->flags & FLAG_INPLACE)) {
    data = calloc (copy_size, 1);

    if (data == NULL) {
      db_log (ipod_db, -errno, "db_build_tree: error allocating %i bytes for data.\n", copy_size);
      perror("db_build_tree|calloc");
      exit (EXIT_FAILURE);
    }

    is_alloced = 1;
    memmove (data, &buffer[bytes_read], copy_size);
  } else {
    /* keep the database in the memory initially allocated */
    data = &buffer[bytes_read];

    if (bytes_read == 0)
      is_alloced = 1;
  }

  tree_node_create (&tnode_0, parent, data, copy_size, is_alloced);

  if (cell->type == DOHM && has_children == 0) {
    struct db_dohm *dohm_data = (struct db_dohm *)&buffer[bytes_read];
    
    /* A dohm cell can hold a string, data, or a sub-tree. Process data/string
       dohm cells in a different way than those that have subtrees. */
    if (dohm_contains_string(dohm_data) != 0) {
      u_int8_t *string_start;
      size_t string_length;
      int utf16 = 0;

      struct db_string_dohm *string_dohm_data = (struct db_string_dohm *)&buffer[bytes_read];
      
      bswap_block (&buffer[bytes_read + 0x18], 4, 3);
      
      /* Read a string dohm */
      if (string_dohm_data->unk5 == 0) {
	struct string_header_16 *string_header = (struct string_header_16 *)&buffer[bytes_read + 0x18];
	/* iTunesDB dohm strings are 16 bytes in size */
	tnode_0->string_header_size = 16;

	string_start = &buffer[bytes_read + 0x28];
	string_length = string_header->string_length;

	bswap_block (&buffer[bytes_read + 0x24], 4, 1);

	/* the iPod can handle UTF-8 strings */
	if (string_header->unk0 != 0)
	  utf16 = 1;
      } else {
	struct string_header_12 *string_header = (struct string_header_12 *)&buffer[bytes_read + 0x18];

	/* ArtworkDB dohm strings are four bytes smaller than those in the iTunesDB */
	tnode_0->string_header_size = 12;

	string_start = &buffer[bytes_read + 0x24];
	string_length = string_header->string_length;

	/* this is a guess based off of the ArtworkDB */
	if (string_header->format != 1)
	  utf16 = 1;
      }

      /* Swap UTF-16 strings */
      if (utf16 == 1)
	bswap_block (string_start, 2, string_length/2);
    } else
      /* data dohm */
      bswap_block (&buffer[bytes_read + cell->cell_size], 4, (cell->subtree_size - cell->cell_size)/4);

  } else if (cell->type == TIHM) {
    struct db_tihm *tihm_data = (struct db_tihm *)&buffer[bytes_read];
    
    ipod_db->last_entry = ((ipod_db->last_entry < tihm_data->identifier)
			  ? tihm_data->identifier
			  : ipod_db->last_entry);
  } else if (cell->type == IIHM) {
    struct db_iihm *iihm_data = (struct db_iihm *)&buffer[bytes_read];

    ipod_db->last_entry = ((ipod_db->last_entry < iihm_data->identifier)
			  ? iihm_data->identifier
			  : ipod_db->last_entry);
  }
  

  /*  db_log (ipod_db, 0, "Loaded tree node: type: %s, size: %08x\n", iptr, copy_size); */

  (*bytes_readp) += copy_size;

  if (has_children != 0) {
    tnode_0->children = calloc(1, sizeof(tree_node_t *));

    if (tnode_0->children == NULL) {
      perror("db_build_tree|calloc");
      exit (EXIT_FAILURE);
    }

    /* build the subtree for the current cell */
    while ((*bytes_readp - bytes_read) < cell->subtree_size) {
      tnode_0->children = realloc(tnode_0->children, ++(tnode_0->num_children) *
				  sizeof(tree_node_t *));
    
      if (tnode_0->children == NULL) {
	perror("db_build_tree|realloc");
	exit (EXIT_FAILURE);
      }

      if ((ret = db_build_tree(ipod_db, &(tnode_0->children[tnode_0->num_children - 1]), bytes_readp,
			       tree_size, tnode_0, buffer)) != 0) {
	return ret;
      }
    }
  }

  *tnode_0p = tnode_0;

  return 0;
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

  size_t bytes_read  = 0;

  if (path == NULL || strlen(path) == 0 || ipod_db == NULL)
    return -EINVAL;

  db_log (ipod_db, 0, "db_load: entering...\n");
  db_log (ipod_db, 0, "db_load: flags: %08x\n", flags);
  ipod_db->flags = flags;

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
    db_log (ipod_db, 0, "db_load: Reading an itunes database.\n");
    ipod_db->type = 0;
  } else if (ibuffer[0] == DFHM) {
    db_log (ipod_db, 0, "db_load: Reading a photo or artwork database.\n");
    ipod_db->type = 1;
  } else {
    bswap_block((char *)ibuffer, 4, 3);

    if (get_uint24 ((u_int8_t *)ibuffer, 1) == 0x010600) {
      db_log (ipod_db, 0, "db_load: File appears to be an itunessd file. Calling sd_load...\n");

      return sd_load (ipod_db, path, flags);
    } else {
      db_log (ipod_db, -1, "db_load: %s is not a valid database. Exiting.\n", path);
      close (iPod_DB_fd);
    
      return -1;
    }
  }

  lseek (iPod_DB_fd, 0, SEEK_SET);
  
  buffer = (char *)calloc(ibuffer[2], 1);
  if (buffer == NULL) {
    db_log (ipod_db, errno, "db_load: Could not allocate memory\n");
    close (iPod_DB_fd);

    return -errno;
  }

  if (ibuffer[2] != statinfo.st_size)
    db_log (ipod_db, -1, "db_load: File size does not match database size! Continuing anyway.\n");

  /* read in the rest of the database */
  if ((ret = read (iPod_DB_fd, buffer, ibuffer[2])) < ibuffer[2]) {
    db_log (ipod_db, errno, "db_load: Short read: %i bytes wanted, %i read\n", ibuffer[2], ret);
    
    free(buffer);
    close(iPod_DB_fd);
    return -1;
  }
  
  close (iPod_DB_fd);

  ipod_db->path  = strdup (path);

  /* Load the database into a tree structure. */
  if ((ret = db_build_tree (ipod_db, &ipod_db->tree_root, &bytes_read, statinfo.st_size, NULL, buffer)) != 0) {
    db_free (ipod_db);

    return ret;
  }

  if (!(flags & FLAG_INPLACE))
    /* free flat database as it is no longer useful */
    free (buffer);

  db_log (ipod_db, 0, "db_load: complete. %i Bytes\n", bytes_read);

  return bytes_read;
}

static int db_write_tree (int fd, tree_node_t *entry) {
  int ret = 0;
  int i, swap;
  struct db_dohm *dohm_data = (struct db_dohm *) entry->data;

  if (dohm_data->dohm == DOHM) {
    if (entry->string_header_size == 16) {
      struct string_header_16 *string_header;

      string_header = (struct string_header_16 *)&entry->data[0x18];

      swap = 10;

      if (string_header->unk0 != 0)
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

  ret += write (fd, entry->data, entry->data_size);

  bswap_block(entry->data, 4, swap);

  if (dohm_data->dohm == DOHM && (dohm_data->type & 0x01000000) == 0) {
    if (entry->string_header_size == 16) {
      struct string_header_16 *string_header;
      string_header = (struct string_header_16 *)&entry->data[0x18];

      if (string_header->unk0 != 0)
	bswap_block (&(entry->data[0x28]), 2, string_header->string_length/2);
    } else if (entry->string_header_size == 12) {
      struct string_header_12 *string_header;
      string_header = (struct string_header_12 *)&entry->data[0x18];

      if (string_header->format != 1)
	bswap_block (&(entry->data[0x24]), 2, string_header->string_length/2);
    }
  }

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

  if (ipod_db.type == 0) {
    db_playlist_strip_indices (&ipod_db);
    db_playlist_add_indices (&ipod_db);
  }

  ret = db_write_tree (fd, ipod_db.tree_root);
  
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
  if (parent->num_children++ == 0) {
    parent->children = calloc (1, sizeof(tree_node_t *));
  } else
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
  (*entry)->data_isalloced = 1;

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
