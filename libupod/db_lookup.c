/**
 *   (c) 2003 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.2 db_lookup.c
 *
 *   Contains function for looking up a tihm entry in the iTunesDB
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

#include <stdlib.h>
#include <stdio.h>

#include "itunesdbi.h"
#include "hexdump.c"

/*
  db_lookup:

  Returns the tihm identifier of the first match to data.
  The data field is in UTF-8 format

  Returns:
     -1 if not found
   < -1 if any error occurred
   >= 0 if found
*/

int db_lookup (itunesdb_t *itunesdb, int dohm_type, char *data, int data_len) {
  struct tree_node *dshm, *tihm, *dohm;
  int i, j, ret;

  /* simpifies code */
  struct db_tlhm *tlhm_data;
  struct db_tihm *tihm_data;
  struct db_dohm *dohm_data;

  size_t unicode_data_len;
  u_int16_t *unicode_data;

  if (db_dshm_retrieve (itunesdb, &dshm, 0x1) != 0)
    return -1;

  to_unicode (&unicode_data, &unicode_data_len, data, data_len, "UTF-8");

  tlhm_data = (struct db_tlhm *)dshm->children[0]->data;

  for (i = 1 ; i <= tlhm_data->num_tihm ; i++) {
    tihm = dshm->children[i];
    tihm_data = (struct db_tihm *)tihm->data;

    /* since there is no garuntee that the dohm entries are
       in any order, do a linear traversal of the list looking for the
       data */
    for (j = 0 ; j < tihm_data->num_dohm ; j++) {
      dohm = tihm->children[j];
      dohm_data = (struct db_dohm *)dohm->data;
      
      if ((dohm_type > -1 && dohm_data->type != dohm_type) ||
	  unicode_data_len != dohm_data->len)
	continue;

      if (memcmp (&dohm->data[0x28], unicode_data, unicode_data_len) == 0) {
	ret = tihm_data->identifier;
	goto found;
      }
    }
  }
 notfound:
  ret = -1;
 found:
  free(unicode_data);
  return ret;
}

/*
  db_lookup_tihm:

  Returns the tihm identifier of the first tihm with a dohm entry of any type
  that exactly matches data.

  The data field can either be in unicode or character format.

  Returns:
     -1 on not found
   < -1 on any error occurred
   >= 0 on found
*/
int db_lookup_tihm (itunesdb_t *itunesdb, char *data, int data_len) {
  return db_lookup (itunesdb, -1, data, data_len);
}

int db_lookup_playlist (itunesdb_t *itunesdb, char *data, int data_len) {
  tree_node_t *dshm_header, *plhm_header, *dohm_header;
  int i, j, ret;
  size_t unicode_data_len;
  u_int16_t *unicode_data;

  /* simpifies code */
  struct db_plhm *plhm_data;
  struct db_dohm *dohm_data;

  if (db_playlist_retrieve_header (itunesdb, &plhm_header, &dshm_header) != 0)
    return -1;

  to_unicode (&unicode_data, &unicode_data_len, data, data_len, "UTF-8");

  plhm_data = (struct db_plhm *)plhm_header->data;

  for (i = 1 ; i < plhm_data->num_pyhm ; i++) {
    dohm_header = dshm_header->children[i+1]->children[1];
    dohm_data = (struct db_dohm *)dohm_header->data;
   
    /* again, exact matches only */
    if (unicode_data_len != dohm_data->len)
      continue;
    
    if (memcmp (&dohm_header->data[0x28], unicode_data, unicode_data_len) == 0) {
      ret = i;
      goto found_playlist;
    }
  }

 notfound_playlist:
  ret = -1;
 found_playlist:
  return ret;
}
