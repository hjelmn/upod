/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.2a dshm.c
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

#define DSHM_HEADER_SIZE 0x60

int db_dshm_retrieve (itunesdb_t *itunesdb, tree_node_t **dshm_header,
		      int type) {
  int i;
  struct tree_node *root;
  struct db_dshm *dshm_data;

  if (itunesdb == NULL || dshm_header == NULL) return -1;
  root = itunesdb->tree_root;
  if (root == NULL) return -1;

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

int db_dshm_create (tree_node_t *entry, int type) {
  struct db_dshm *dshm_data;

  if (entry == NULL) return -1;

  memset (entry, 0, sizeof (tree_node_t));
  
  entry->size = DSHM_HEADER_SIZE;
  entry->data = malloc (entry->size);
  memset (entry->data, 0, entry->size);
  dshm_data = (struct db_dshm *) entry->data;

  dshm_data->dshm = DSHM;
  dshm_data->header_size = DSHM_HEADER_SIZE;
  dshm_data->record_size = DSHM_HEADER_SIZE;
  dshm_data->type        = type;
}
