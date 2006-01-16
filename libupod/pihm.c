/**
 *   (c) 2002-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.4.0 pihm.c
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
		    u_int32_t order, u_int32_t podcast_group) {
  struct db_pihm *pihm_data;
  tree_node_t *dohm_header;
  int ret;

  if ((ret = db_node_allocate (entry, PIHM, PIHM_CELL_SIZE, PIHM_CELL_SIZE)) < 0)
    return ret;

  pihm_data = (struct db_pihm *)(*entry)->data;
  
  pihm_data->num_dohm  = 1;
  pihm_data->order     = order;
  pihm_data->reference = tihm_num;
  pihm_data->podcast_group = podcast_group;

  pihm_data->date_added = DATE_TO_APPLE(time (NULL));

  db_dohm_create_pihm (&dohm_header, order);

  db_attach (*entry, dohm_header);

  return 0;
}

/* creates a grouping pihm/dohm for a podcast */
int db_pihm_create_podcast (struct tree_node **entry, u_int8_t *podcast_name,
			    u_int32_t podcast_group) {
  struct db_pihm *pihm_data;
  tree_node_t *dohm_header;
  dohm_t dohm;
  int ret;

  if ((ret = db_node_allocate (entry, PIHM, PIHM_CELL_SIZE, PIHM_CELL_SIZE)) < 0)
    return ret;

  pihm_data = (struct db_pihm *)(*entry)->data;
  
  pihm_data->num_dohm = 1;
  pihm_data->order    = podcast_group;


  /* create podcast title dohm */
  pihm_data->flag     = 0x00000100;

  dohm.type = IPOD_TITLE;
  dohm.data = podcast_name;

  db_dohm_create (&dohm_header, dohm, 16, 0);
  db_attach (*entry, dohm_header);

  return 0;  
}
