/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 pyhm.c
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

#include "itunesdbi.h"


#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include <time.h>

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

int db_pyhm_dohm_attach (tree_node_t *entry, tree_node_t *dohm) {
  struct db_pyhm *pyhm_data;
  
  pyhm_data = (struct db_pyhm *)entry->data;
  pyhm_data->num_dohm++;

  db_attach (entry, dohm);

  return 0;
}

