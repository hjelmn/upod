/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 dshm.c
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

#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#define DSHM_HEADER_SIZE 0x60

/* An itunesdb has two dshm entries. An artworkdb has three. */
int db_dshm_retrieve (ipoddb_t *ipod_db, tree_node_t **dshm_header,
		      int type) {
  int i;
  struct tree_node *root;
  struct db_dshm *dshm_data;

  if (ipod_db == NULL || dshm_header == NULL || ipod_db->tree_root == NULL)
    return -EINVAL;

  root = ipod_db->tree_root;

  for (i = 0 ; i < root->num_children ; i++) {
    *dshm_header = (tree_node_t *) root->children[i];
    dshm_data = (struct db_dshm *) (*dshm_header)->data;
    
    if (dshm_data->dshm == DSHM && dshm_data->type == type)
      break;
  }

  if (i == root->num_children){
    *dshm_header = NULL;
    return -1;
  }

  return 0;
}

int db_dshm_create (tree_node_t **entry, int type) {
  struct db_dshm *dshm_data;
  int ret;

  if ((ret = db_node_allocate (entry, DSHM, DSHM_HEADER_SIZE, DSHM_HEADER_SIZE)) < 0)
    return ret;

  dshm_data = (struct db_dshm *)(*entry)->data;
  dshm_data->type  = type;

  return 0;
}
