/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 db_lookup.c
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

int db_lookup (ipoddb_t *itunesdb, int dohm_type, char *data) {
  struct tree_node *dshm_header, *tihm_header, *dohm_header, *dohm_temp;
  int i, ret;

  /* simpifies code */
  struct db_tlhm *tlhm_data;
  struct db_tihm *tihm_data;
  dohm_t dohm;

  db_log (itunesdb, 0, "db_lookup: entering...\n");

  if ((ret = db_dshm_retrieve (itunesdb, &dshm_header, 0x1)) != 0)
    return ret;

  dohm.data = data;
  dohm.type = dohm_type;

  db_dohm_create (&dohm_temp, dohm, 16, itunesdb->flags);

  tlhm_data = (struct db_tlhm *)dshm_header->children[0]->data;

  ret = -1;

  for (i = 1 ; i <= tlhm_data->num_tihm ; i++) {
    tihm_header = dshm_header->children[i];
    tihm_data = (struct db_tihm *)tihm_header->data;

    db_dohm_retrieve (tihm_header, &dohm_header, dohm_type);

    if (db_dohm_compare (dohm_temp, dohm_header) == 0) {
      ret = tihm_data->identifier;
      break;
    }
  }

  if (ret != -1)
    db_log (itunesdb, 0, "db_lookup: found\n");
  else
    db_log (itunesdb, 0, "db_lookup: not found\n");

  db_free_tree (dohm_temp);

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
int db_lookup_tihm (ipoddb_t *itunesdb, char *data) {
  return db_lookup (itunesdb, -1, data);
}

int db_lookup_playlist (ipoddb_t *itunesdb, char *data) {
  tree_node_t *dshm_header, *dohm_header = NULL;
  int i, j, ret;
  size_t unicode_data_len;
  u_int16_t *unicode_data;

  int *iptr;

  /* simpifies code */
  struct db_plhm *plhm_data;
  struct db_dohm *dohm_data = NULL;

  db_log (itunesdb, 0, "db_lookup_playlist: entering...\n");

  if (db_playlist_retrieve (itunesdb, &plhm_data, &dshm_header, 0, NULL) != 0)
    return -1;

  to_unicode (&unicode_data, &unicode_data_len, data, strlen(data), "UTF-8");

  if (itunesdb->log_level > 1)
    pretty_print_block ((unsigned char *)unicode_data, unicode_data_len);

  db_log (itunesdb, 0, "db_lookup_playlist: number of playlists = %i\n", plhm_data->num_pyhm);

  for (i = 1 ; i < plhm_data->num_pyhm+1 ; i++) {
    dohm_header = NULL;

    for (j = 0 ; j < dshm_header->children[i]->num_children ; j++) {
      dohm_header = dshm_header->children[i]->children[j];
      dohm_data = (struct db_dohm *)dohm_header->data;

      if (dohm_data->dohm == DOHM && dohm_data->type == IPOD_TITLE)
	break;
    }

    if (dohm_header == NULL)
      continue;

    iptr = (int *)dohm_data;

    if (itunesdb->log_level > 1)
      pretty_print_block (&dohm_header->data[0x28], (iptr[7] == 0) ? 2 : iptr[7]);

    /* again, exact matches only */
    if (unicode_data_len != iptr[7])
      continue;
    
    if (memcmp (&dohm_header->data[0x28], unicode_data, unicode_data_len) == 0) {
      ret = i-1;
      goto found_playlist;
    }
  }

  db_log (itunesdb, 0, "db_lookup_playlist: not found\n");

  ret = -1;
 found_playlist:
  if (ret != -1)
    db_log (itunesdb, 0, "db_lookup_playlist: found\n");

  free (unicode_data);

  return ret;
}
