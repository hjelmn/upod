/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a tihm.c
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "upodi.h"

#include <stdlib.h>
#include <stdio.h>

#define PIHM               0x7069686d
#define PIHM_HEADER_SIZE   0x4c
#define PIHM_TOTAL_SIZE    0x4c

int db_pihm_search (struct tree_node *entry, u_int32_t tihm_num) {
  int i;

  for ( i = 2 ; i < entry->num_children ; i++ )
    if (((int *)(entry->children[i]->data))[6] == tihm_num)
      return i;

  return -1;
}

int db_pihm_create (struct tree_node *entry, u_int32_t tihm_num, u_int32_t junk) {
  int *iptr;
  
  entry->size = PIHM_HEADER_SIZE;
  entry->data = calloc (1, PIHM_HEADER_SIZE);
  entry->num_children = 0;
  entry->children     = NULL;

  iptr = (int *)entry->data;

  iptr[0] = PIHM;
  iptr[1] = PIHM_HEADER_SIZE;
  iptr[2] = PIHM_TOTAL_SIZE;
  iptr[3] = 1;
  iptr[5] = junk + 1;
  iptr[6] = tihm_num;
  iptr[7] = 0xb864adba;

  return 0;
}
