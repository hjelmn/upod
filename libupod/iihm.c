/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 iihm.c
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

#define IIHM_HEADER_SIZE 0x98

int db_iihm_create (tree_node_t **entry, int identifier) {
  struct db_iihm *iihm_data;
  int ret;

  if ((ret = db_node_allocate (entry, IIHM, IIHM_HEADER_SIZE, IIHM_HEADER_SIZE)) < 0)
    return ret;

  iihm_data = (struct db_iihm *)(*entry)->data;
  iihm_data->identifier  = identifier;
}
