/**
 *   (c) 2002-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 aihm.c
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

#include "itunesdbi.h"

int db_aihm_search (struct tree_node *entry, u_int32_t image_id) {
  int i;
  struct db_aihm *aihm_data;
  struct db_abhm *abhm_data;

  abhm_data = (struct db_abhm *)entry->data;

  for (i = abhm_data->num_dohm ; i < entry->num_children ; i++ ) {
    aihm_data = (struct db_aihm *)entry->children[i]->data;
    if (aihm_data->reference == image_id)
      return i;
  }

  return -1;
}

int db_aihm_create (struct tree_node **entry, u_int32_t image_id) {
  struct db_aihm *aihm_data;
  int ret;

  if ((ret = db_node_allocate (entry, AIHM, AIHM_CELL_SIZE, AIHM_CELL_SIZE)) < 0)
    return ret;

  aihm_data = (struct db_aihm *)(*entry)->data;
  aihm_data->reference = image_id;

  return 0;
}
