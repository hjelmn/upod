/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 pyhm.c
 *
 *   Functions for managing playlists on the iPod.
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

#include <time.h>

#include "itunesdbi.h"

#define PYHM_HEADER_SIZE 0x6c

int db_pyhm_create (tree_node_t **entry, int is_visible) {
  struct db_pyhm *pyhm_data;
  int ret;

  if ((ret = db_node_allocate (entry, PYHM, PYHM_HEADER_SIZE, PYHM_HEADER_SIZE)) < 0)
    return ret;

  pyhm_data = (struct db_pyhm *)(*entry)->data;
  pyhm_data->is_visible    = (is_visible) ? 1 : 0;
  pyhm_data->creation_date = time (NULL);

  return 0;
}

int db_pyhm_set_id (tree_node_t *entry, int id) {
  struct db_pyhm *pyhm_data;

  pyhm_data = (struct db_pyhm *)entry->data;
  pyhm_data->playlist_id = id;

  return 0;
}

int db_pyhm_get_id (tree_node_t *entry) {
  struct db_pyhm *pyhm_data;

  pyhm_data = (struct db_pyhm *)entry->data;
  return pyhm_data->playlist_id;
}

int db_pyhm_dohm_attach (tree_node_t *entry, tree_node_t *dohm) {
  struct db_pyhm *pyhm_data;
  
  pyhm_data = (struct db_pyhm *)entry->data;
  pyhm_data->num_dohm++;

  db_attach_at (entry, dohm, pyhm_data->num_dohm - 1);

  return 0;
}

int db_pyhm_dohm_detach (tree_node_t *pyhm_header, int index, tree_node_t **store) {
  struct db_pyhm *pyhm_data;
  
  pyhm_data = (struct db_pyhm *)pyhm_header->data;
  pyhm_data->num_dohm--;

  return db_detach (pyhm_header, index, store);
}
