/**
 *   (c) 2002-2004 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v1.0 create.c
 *
 *   Contains db_create and various node creation routines (that dont have another home).
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

#define DBHM_HEADER_SIZE 0x68
#define TLHM_HEADER_SIZE 0x5c
#define PLHM_HEADER_SIZE 0x5c

static int db_dbhm_create (tree_node_t *entry) {
  struct db_dbhm *dbhm_data;

  if (entry == NULL) return -1;

  memset (entry, 0, sizeof (tree_node_t));
  
  entry->size = DBHM_HEADER_SIZE;
  entry->data = calloc (entry->size, sizeof(char));
  memset (entry->data, 0, entry->size);
  dbhm_data = (struct db_dbhm *) entry->data;

  dbhm_data->dbhm = DBHM;
  dbhm_data->header_size = DBHM_HEADER_SIZE;
  dbhm_data->db_size = DBHM_HEADER_SIZE;
  
  /* these are the values seen in iTunes iTunesDBs */
  dbhm_data->unk0 = 0x1;
  dbhm_data->unk1 = 0x1;
  dbhm_data->unk2 = 0x2;
}

static int db_tlhm_create (tree_node_t *entry) {
  struct db_tlhm *tlhm_data;

  if (entry == NULL) return -1;

  memset (entry, 0, sizeof (tree_node_t));
  
  entry->size = TLHM_HEADER_SIZE;
  entry->data = calloc (entry->size, sizeof(char));
  memset (entry->data, 0, entry->size);
  tlhm_data = (struct db_tlhm *) entry->data;

  tlhm_data->tlhm = TLHM;
  tlhm_data->header_size = TLHM_HEADER_SIZE;
}

static int db_plhm_create (tree_node_t *entry) {
  struct db_plhm *plhm_data;

  if (entry == NULL) return -1;

  memset (entry, 0, sizeof (tree_node_t));
  
  entry->size = PLHM_HEADER_SIZE;
  entry->data = calloc (entry->size, sizeof(char));
  memset (entry->data, 0, entry->size);
  plhm_data = (struct db_plhm *) entry->data;

  plhm_data->plhm = PLHM;
  plhm_data->header_size = PLHM_HEADER_SIZE;
}

/**
  db_create:

  Creates an empty itunesdb.

  Arguments:
   itunesdb_t *itunesdb - pointer to where the database should be loaded.
   char       *db_name  - name of this database, usually the name of your ipod
   int         name_len - length of db_name

  Returns:
   < 0 on error
     0 on success
**/
int db_create (itunesdb_t *itunesdb, char *db_name, int name_len, int flags) {
  tree_node_t *root, *entry, *entry2;
  int ret;

  db_log (itunesdb, 0, "db_create: entering...\n");

  if (itunesdb == NULL || db_name == NULL || name_len < 1)
    return -EINVAL;

  root = itunesdb->tree_root;

  if (root != NULL)
    db_free (itunesdb);

  memset (itunesdb, 0, sizeof (itunesdb_t));

  root = itunesdb->tree_root = (tree_node_t *) calloc (1, sizeof(tree_node_t));
  db_dbhm_create (root);

  /* create song list */
  entry = (tree_node_t *) calloc (1, sizeof(tree_node_t));
  db_dshm_create (entry, 1); /* type 1 is song list */
  db_attach (root, entry);

  entry2 = (tree_node_t *) calloc (1, sizeof(tree_node_t));
  db_tlhm_create (entry2);
  db_attach (entry, entry2);

  /* create master playlist */
  entry = (tree_node_t *) calloc (1, sizeof(tree_node_t));
  db_dshm_create (entry, 2); /* type 2 is playlist list */
  db_attach (root, entry);

  entry2 = (tree_node_t *) calloc (1, sizeof(tree_node_t));
  db_plhm_create (entry2);
  db_attach (entry, entry2);

  itunesdb->flags = flags;
  itunesdb->last_tihm = 0;
  
  ret = db_playlist_create (itunesdb, db_name, name_len);

  db_log (itunesdb, 0, "db_create: complete\n");

  return ret;
}
