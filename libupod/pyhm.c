/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.1a pyhm.c
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

#define PYHM_HEADER_SIZE 0x6c

int db_pyhm_create (tree_node_t *entry) {
  struct db_pyhm *pyhm_data;
  if (entry == NULL) return -1;

  memset (entry, 0, sizeof(tree_node_t));

  entry->size = PYHM_HEADER_SIZE;
  entry->data = calloc (PYHM_HEADER_SIZE, 1);
  if (entry->data == NULL) {
    perror ("db_pyhm_create|calloc");
    return -errno;
  }

  pyhm_data = (struct db_pyhm *)entry->data;
  pyhm_data->pyhm        = PYHM;
  pyhm_data->header_size = PYHM_HEADER_SIZE;
  pyhm_data->record_size = PYHM_HEADER_SIZE;

  /* Identifies the dohm that contains the name */
  pyhm_data->name_index  = 0x2;
}
