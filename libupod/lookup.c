/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.2a lookup.c
 *
 *   Contains function for looking up an tihm entry in the iTunesDB
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

/*
  db_lookup_tihm:

  Returns the tihm identifier of the first match to data.
  The data field can either be in unicode or character format.

  Returns:
     -1 on not found
   < -1 on any error occurred
   >= 0 on found
*/

int db_lookup_tihm (itunesdb_t *itunesdb, int dohm_type, char *data,
		    int data_len) {
  struct tree_node *dshm_header, *tihm_header, *root, *dohm_header;
  int i, j, ret = -1, unicode_data_len;
  char *unicode_data;

  /* simpifies code */
  struct db_tlhm *tlhm_data;
  struct db_tihm *tihm_data;
  struct db_dohm *dohm_data;
  struct db_dshm *dshm_data;

  if (itunesdb == NULL) return -1;
  root = itunesdb->tree_root;
  if (root == NULL)     return -1;

  unicode_check_and_copy (&unicode_data, &unicode_data_len, data, data_len);

  /* find the song list */
  for (i = 0 ; i < root->num_children ; i++) {
    dshm_header = (tree_node_t *)root->children[i];
    dshm_data = (struct db_dshm *) dshm_header->data;
    
    if (dshm_data->dshm == DSHM && dshm_data->type == 0x1)
      break;
  }
  if (i == root->num_children) goto notfound_tihm;

  tlhm_data = (struct db_tlhm *)dshm_header->children[0]->data;

  for (i = 1 ; i <= tlhm_data->num_tihm ; i++) {
    tihm_header = dshm_header->children[i];

    tihm_data = (struct db_tihm *)tihm_header->data;

    /* try to retrive the dohm we want */
    if (db_dohm_retrieve (tihm_header, &dohm_header, dohm_type) < 0)
      continue;

    dohm_data = (struct db_dohm *)dohm_header->data;

    /* we are looking for exact matches */
    if (dohm_data->len != unicode_data_len) continue;

    if (memmem (&dohm_header->data[0x28], dohm_data->len,
		unicode_data, unicode_data_len) != 0) {
      ret = tihm_data->identifier;
      goto found_tihm;
    }
  }

 notfound_tihm: /* if ret == -1 */
 found_tihm:
  free(unicode_data);
  return ret;
}

int db_lookup_playlist (itunesdb_t *itunesdb, char *data, int data_len) {
  tree_node_t *dshm_header, *plhm_header, *dohm_header;
  int i, j, ret = -1, unicode_data_len;
  char *unicode_data;

  /* simpifies code */
  struct db_plhm *plhm_data;
  struct db_dohm *dohm_data;

  if (itunesdb == NULL) return -1;

  unicode_check_and_copy (&unicode_data, &unicode_data_len, data, data_len);

  db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header);

  plhm_data = (struct db_plhm *)plhm_header->data;

  for (i = 1 ; i < plhm_data->num_pyhm ; i++) {
    dohm_header = dshm_header->children[i+1]->children[1];
    dohm_data = (struct db_dohm *)dohm_header->data;
   
    /* again, exact matches only */
    if (unicode_data_len != dohm_data->len)
      continue;
    
    if (memmem (&dohm_header->data[0x28], dohm_data->len, unicode_data,
		unicode_data_len) != 0) {
      ret = i;
      goto found_playlist;
    }
  }

 notfound_playlist:
  ret = -1;
 found_playlist:
  return ret;
}
