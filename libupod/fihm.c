/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 fihm.c
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

#include <sys/types.h>
#include <fcntl.h>

#include <sys/stat.h>

#include "itunesdbi.h"

#define FIHM_HEADER_SIZE 0x7c

int db_fihm_create (tree_node_t **entry, unsigned int file_id) {
  struct db_fihm *fihm_data;
  int ret;

  if ((ret = db_node_allocate (entry, FIHM, FIHM_HEADER_SIZE, FIHM_HEADER_SIZE)) < 0)
    return ret;

  fihm_data = (struct db_fihm *)(*entry)->data;

  fihm_data->file_id = file_id;

  return 0;
}

int db_fihm_find (ipoddb_t *photodb, unsigned int file_id) {
  struct tree_node *dshm_header;
  struct db_flhm *flhm_data;
  int ret, i;

  /* find the file list */
  if ((ret = db_dshm_retrieve (photodb, &dshm_header, 3)) < 0) {
    db_log (photodb, ret, "Could not get file list header\n");
    return ret;
  }

  flhm_data = (struct db_flhm *)dshm_header->children[0]->data;

  for (i = 0 ; i < flhm_data->num_files ; i++) {
    struct db_fihm *fihm_data = (struct db_fihm *)dshm_header->children[i+1]->data;
    
    if (fihm_data->file_id == file_id)
      return i+1;
  }

  return 0;
}

int db_fihm_register (ipoddb_t *photodb, char *file_name, unsigned long file_id) {
  struct tree_node *dshm_header, *new_fihm_header;
  struct db_flhm *flhm_data;
  struct db_fihm *fihm_data;
  int ret;
  struct stat statinfo;

  /* find the file list */
  if ((ret = db_dshm_retrieve (photodb, &dshm_header, 3)) < 0) {
    db_log (photodb, ret, "Could not get file list header\n");
    return ret;
  }

  if ((ret = db_fihm_find (photodb, file_id)) == 0) {
    flhm_data = (struct db_flhm *)dshm_header->children[0]->data;
    flhm_data->num_files++;
    
    db_fihm_create (&new_fihm_header, file_id);
    db_attach (dshm_header, new_fihm_header);
  } else
    new_fihm_header = dshm_header->children[ret];

  fihm_data = (struct db_fihm *)new_fihm_header->data;

  stat (file_name, &statinfo);
  fihm_data->file_size = statinfo.st_size;

  return 0;
}
