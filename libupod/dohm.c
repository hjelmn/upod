/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a dohm.c
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

#define DOHM_HEADER_SIZE   0x18

#define DOHM_EQ_SIZE       0x3a

#define STRING_HEADER_SIZE 0x10

int db_dohm_retrieve (tree_node_t *tihm_header, tree_node_t **dohm_header,
		      int dohm_type) {
  int i;
  struct db_dohm *dohm_data;

  if (dohm_header == NULL) return -1;
  
  for (i = 0 ; i < tihm_header->num_children ; i++) {
    dohm_data = (struct db_dohm *)tihm_header->children[i]->data;
    if (dohm_data->type == dohm_type) {
      *dohm_header = tihm_header->children[i];
      return i;
    }
  }

  *dohm_header = 0;
  return -1;
}

/* 
   this function was created just because this needs to be used ALOT when
   preparing to add a song to the iTunesDB.

   it creates a new tree leaf and appends it to the dohm list in the tihm
   structure
*/
dohm_t *dohm_create (tihm_t *tihm) {
  dohm_t *dohm;

  if (tihm == NULL)
    return NULL;

  if (tihm->num_dohm++ == 0)
    tihm->dohms = (dohm_t *)malloc(sizeof(dohm_t));
  else
    tihm->dohms = realloc(tihm->dohms, tihm->num_dohm * sizeof(dohm_t));
  
  dohm = &(tihm->dohms[tihm->num_dohm - 1]);
  memset (dohm, 0, sizeof(dohm_t));

  return dohm;
}

int dohm_add (tihm_t *timh, char *data, int data_len, int data_type) {
  dohm_t *dohm;

  if ((dohm = dohm_create (timh)) == NULL)
    return -1;

  dohm->type = data_type;
  unicode_check_and_copy ((char **)&(dohm->data), &(dohm->size), data, data_len);

  return 0;
}

/* removes the last dohm from the list in the tihm structure */
void dohm_destroy (tihm_t *tihm) {
  dohm_t *dohm;

  if (tihm == NULL || tihm->num_dohm == 0)
    return;

  tihm->num_dohm--;

  dohm = &(tihm->dohms[tihm->num_dohm]);
  
  if (dohm->data)
    free(dohm->data);

  tihm->dohms = realloc(tihm->dohms, tihm->num_dohm * sizeof(dohm_t));
}

int db_dohm_create_generic (tree_node_t *entry, size_t size, int type) {
  struct db_dohm *dohm_data;
  
  entry->size         = size;
  entry->data         = malloc(size);
  memset (entry->data, 0, size);
  entry->num_children = 0;
  entry->children     = NULL;

  dohm_data = (struct db_dohm *)entry->data;

  dohm_data->dohm        = DOHM;
  dohm_data->header_size = DOHM_HEADER_SIZE;
  dohm_data->record_size = size;
  dohm_data->type        = type;

  return 0;
}

int db_dohm_create (tree_node_t *entry, dohm_t dohm) {
  int *iptr;
  int entry_size;

  entry->parent = NULL;
  entry_size   = DOHM_HEADER_SIZE + STRING_HEADER_SIZE + dohm.size;

  db_dohm_create_generic (entry, entry_size, dohm.type);

  iptr = (int *)entry->data;
  iptr[DOHM_HEADER_SIZE/4]     = 1;
  iptr[DOHM_HEADER_SIZE/4 + 1] = dohm.size;

  memcpy(&entry->data[DOHM_HEADER_SIZE + STRING_HEADER_SIZE], dohm.data,
	 dohm.size);
  
  return entry->size;
}

/*
  db_dohm_create_eq:

  Creates a generic eq dohm for use with a song entry.

  Returns:
   < 0 if an error occured
     0 if successful
*/
int db_dohm_create_eq (tree_node_t *entry, int eq) {
  int *iptr;
  char ceq[] = "\x23\x21\x23\x31\x30\x00\x23\x21\x23";

  if (entry->data != NULL) free(entry->data);

  db_dohm_create_generic(entry, DOHM_EQ_SIZE, IPOD_EQ);
  iptr = (int *)entry->data;
    
  iptr[6] = 0x01;
  iptr[7] = 0x12;

  ceq[5] = eq;
  char_to_unicode (&entry->data[0x28], ceq, 9);

  return 0;
}

dohm_t *db_dohm_fill (tree_node_t *entry) {
  dohm_t *dohms;
  struct db_dohm *dohm_data;
  tree_node_t **dohm_list;
  int i;

  dohms = (dohm_t *) malloc (entry->num_children * sizeof(dohm_t));
  memset(dohms, 0, entry->num_children * sizeof(dohm_t));

  dohm_list = entry->children;

  for (i = 0 ; i < entry->num_children ; i++) {
    dohm_data = (struct db_dohm *)dohm_list[i]->data;

    dohms[i].type = dohm_data->type;
    dohms[i].size = dohm_data->len;
    dohms[i].data = (u_int16_t *) malloc (dohms[i].size);
    memcpy(dohms[i].data, &(dohm_list[i]->data[40]), dohms[i].size);
  }

  return dohms;
}

void dohm_free (dohm_t *dohm, int num_dohms) {
  int i;

  for (i = 0 ; i < num_dohms ; i++)
    free(dohm[i].data);

  free(dohm);
}

/**
  db_dohm_tihm_modify:

   Modifies (or adds) a dohm in (to) a song entry.

  Arguments:
   itunesdb_t *itunesdb - opened itunesdb
   int         tihm_num - song entry to be modified
   dohm_t     *dohm     - structure containing information to be changed

  Returns:
   < 0 on error
     0 on success
**/
int db_dohm_tihm_modify (itunesdb_t *itunesdb, int tihm_num, dohm_t *dohm) {
  tree_node_t *tihm_header, *dohm_header, *entry;
  struct db_dohm *dohm_data;

  int entry_num;

  if (itunesdb == NULL) return -1;
  if (dohm == NULL) return -2;

  if (db_tihm_retrieve (itunesdb, &tihm_header, NULL, tihm_num) < 0)
    return -1;

  if ((entry_num = db_dohm_retrieve (tihm_header, &dohm_header, dohm->type)) <
      0) {
    dohm_header = (tree_node_t *) malloc (sizeof (tree_node_t));
  } else {
    /* the tree is never deep, so this ends up costing theta(1) extra */
    db_detach (tihm_header, entry_num, &dohm_header);

    free (dohm_header->data);
    memset (dohm_header, 0, sizeof (tree_node_t));
  }

  db_dohm_create (dohm_header, *dohm);
  db_attach (tihm_header, dohm_header);

  return 0;
}
