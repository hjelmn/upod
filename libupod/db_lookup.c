/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 db_lookup.c
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

/*
  db_lookup:

  Returns the tihm identifier of the first match to data.
  The data field is in UTF-8 or ASCII format

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
  tree_node_t *dohm_temp, *pyhm_header;
  int i, ret;

  /* simpifies code */
  struct db_plhm *plhm_data;

  dohm_t dohm;
  
  db_log (itunesdb, 0, "db_lookup_playlist: entering...\n");

  if ((ret = db_playlist_retrieve (itunesdb, &plhm_data, &dshm_header, 0, NULL)) != 0)
    return ret;

  dohm.type = IPOD_TITLE;
  dohm.data = data;

  db_dohm_create (&dohm_temp, dohm, 16, itunesdb->flags);

  db_log (itunesdb, 0, "db_lookup_playlist: number of playlists = %i\n", plhm_data->num_pyhm);

  ret = -1;

  for (i = 1 ; i <= plhm_data->num_pyhm ; i++) {
    pyhm_header = dshm_header->children[i];

    db_dohm_retrieve (pyhm_header, &dohm_header, IPOD_TITLE);

    if (db_dohm_compare (dohm_temp, dohm_header) == 0) {
      ret = i - 1;

      break;
    }
  }

  if (ret != -1)
    db_log (itunesdb, 0, "db_lookup_playlist: found\n");
  else
    db_log (itunesdb, 0, "db_lookup_playlist: not found\n");

  db_free_tree (dohm_temp);

  return ret;
}
