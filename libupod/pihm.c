/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 pihm.c
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

#include <time.h>

#define PIHM_HEADER_SIZE   0x4c

int db_pihm_search (struct tree_node *entry, u_int32_t tihm_num) {
  int i;
  struct db_pihm *pihm_data;
  struct db_pyhm *pyhm_data;

  pyhm_data = (struct db_pyhm *)entry->data;

  for (i = pyhm_data->num_dohm ; i < entry->num_children ; i ++) {
    pihm_data = (struct db_pihm *)entry->children[i]->data;
    if (pihm_data->pihm == PIHM && pihm_data->reference == tihm_num)
      return i;
  }

  return -1;
}

int db_pihm_create (struct tree_node **entry, u_int32_t tihm_num,
		    u_int32_t order) {
  struct db_pihm *pihm_data;
  int ret;

  if ((ret = db_node_allocate (entry, PIHM, PIHM_HEADER_SIZE, PIHM_HEADER_SIZE)) < 0)
    return ret;

  pihm_data = (struct db_pihm *)(*entry)->data;
  
  pihm_data->unk0      = 1;
  pihm_data->order     = order;
  pihm_data->reference = tihm_num;

  pihm_data->date_added = time (NULL);

  return 0;
}
