/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 dshm.c
 *
 *   Contains functions for getting and creating a dshm node.
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

/* An itunesdb has two dshm entries. An artworkdb has three. */
int db_dshm_retrieve (ipoddb_t *ipod_db, tree_node_t **dshm_header, int index) {
  struct db_dbhm *dbhm_data;
 
  if (ipod_db == NULL || dshm_header == NULL || ipod_db->tree_root == NULL)
    return -EINVAL;

  dbhm_data = (struct db_dbhm *)ipod_db->tree_root->data;

  if (dbhm_data->num_dshm < index) {
    *dshm_header = NULL;
    return -1;
  }

  *dshm_header = ipod_db->tree_root->children[index - 1];
  
  return 0;
}

int db_dshm_add (ipoddb_t *ipod_db, u_int32_t list_type) {
  struct db_dshm *dshm_data;
  struct db_dbhm *dbhm_data;
  tree_node_t *list_header, *dshm;

  dbhm_data = (struct db_dbhm *)ipod_db->tree_root->data;
  dbhm_data->num_dshm += 1;

  db_dshm_create (&dshm);
  dshm_data = (struct db_dshm *)dshm->data;
  dshm_data->index = dbhm_data->num_dshm;

  db_attach (ipod_db->tree_root, dshm);

  if (list_type != NULL) {
    db_node_allocate (&list_header, list_type, 0x5c, 0);
    db_attach (dshm, list_header);
  }

  return 0;
}
