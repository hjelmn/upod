/**
 *   (c) 2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 abhm.c
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

int db_abhm_create (tree_node_t **entry) {
  struct db_pyhm *pyhm_data;
  int ret;

  if ((ret = db_node_allocate (entry, ABHM, ABHM_CELL_SIZE, ABHM_CELL_SIZE)) < 0)
    return ret;

  pyhm_data = (struct db_pyhm *)(*entry)->data;

  return 0;
}

int db_abhm_dohm_attach (tree_node_t *entry, tree_node_t *dohm) {
  struct db_abhm *abhm_data;
  
  abhm_data = (struct db_abhm *)entry->data;
  abhm_data->num_dohm++;

  db_attach (entry, dohm);

  return 0;
}

int db_abhm_aihm_attach (tree_node_t *entry, tree_node_t *aihm) {
  struct db_abhm *abhm_data;
  
  abhm_data = (struct db_abhm *)entry->data;
  abhm_data->num_aihm++;

  db_attach (entry, aihm);

  return 0;
}
