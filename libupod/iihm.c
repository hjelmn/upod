/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 iihm.c
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

#include <errno.h>

#include "itunesdbi.h"

#define IIHM_HEADER_SIZE 0x98

int db_iihm_create (tree_node_t **entry, int identifier, u_int64_t id) {
  struct db_iihm *iihm_data;
  int ret;

  if ((ret = db_node_allocate (entry, IIHM, IIHM_HEADER_SIZE, IIHM_HEADER_SIZE)) < 0)
    return ret;

  iihm_data = (struct db_iihm *)(*entry)->data;
  iihm_data->identifier  = identifier;
  iihm_data->id = id;

  return 0;
}

/*
  db_iihm_search:

  Searches for an image item with identifier == iihm_identifier.

  If the image item is found the index where it is located is returned.
  Otherwise -1 is returned.
*/
int db_iihm_search (tree_node_t *entry, u_int32_t iihm_identifier) {
  int i;

  for ( i = 1 ; i < entry->num_children ; i++ ) {
    struct db_iihm *iihm_data = (struct db_iihm *)entry->children[i]->data;

    if (iihm_data->iihm == IIHM && iihm_data->identifier == iihm_identifier)
      return i;
  }

  return -1;
}

int db_iihm_retrieve (ipoddb_t *photodb, tree_node_t **entry,
                      tree_node_t **parent, int iihm_identifier) {
  tree_node_t *dshm_header;
  int entry_num;

  int ret;

  db_log (photodb, 0, "db_iihm_retrieve: entering...\n");

  if ((ret = db_dshm_retrieve (photodb, &dshm_header, 1)) < 0) {
    db_log (photodb, ret, "db_iihm_retrieve: Could not get image list header\n");

    return ret;
  }

  if (photodb->type != 1) {
    db_log (photodb, -EINVAL, "db_iihm_retrieve: Called on an iTunesDB.\n");

    return -EINVAL;
  }

  entry_num = db_iihm_search (dshm_header, iihm_identifier);

  if (entry_num != -1) {
    if (entry)
      *entry = dshm_header->children[entry_num];
    
    if (parent)
      *parent= dshm_header;
  }

  db_log (photodb, 0, "db_iihm_retrieve: complete\n");

  return entry_num;
}
