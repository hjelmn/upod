/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a db_lookup.c
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#define ipod_t itunesdb_t

#include "itunesdbi.h"
#include "hexdump.c"

#include <stdlib.h>
#include <stdio.h>

/*
  db_lookup:

  Returns the tihm identifier of the first match to data.
  The data field can either be in unicode or character format.

  Returns:
     -1 if not found
   < -1 if any error occurred
   >= 0 if found
*/

int db_lookup (ipod_t ipod, int dohm_type, char *data, int data_len) {
  struct tree_node **master, *root, *entry;
  int i, j, ret = -1, unicode_data_len;
  char *unicode_data;

  /* simpifies code */
  struct db_tlhm *tlhm;
  struct db_tihm *tihm;
  struct db_dohm *dohm;

  root = ipod.tree_root;
  if (root == NULL) return -1;

  if (data[1] != 0){ /* Data is not in unicode format */
    unicode_data_len = data_len * 2;
    unicode_data = malloc (data_len * 2);
    char_to_unicode(unicode_data, data, data_len);
  } else {
    unicode_data_len = data_len;
    unicode_data = malloc (data_len);
    memcpy (unicode_data, data, data_len);
  }

  /* the playlist that contains the song entries is the first one that
     will be encountered by this loop */
  for (master = &(root->children[0]) ;
       !strstr((*master)->data, "dshm") ; master++);

  tlhm = (struct db_tlhm *)(*master)->children[0]->data;

  for (i = 1 ; i <= tlhm->num_tihm ; i++) {
    entry = (*master)->children[i];

    tihm = (struct db_tihm *)entry->data;

    /* since there is no garuntee that the dohm entries are
       in any order, do a linear traversal of the list looking for the
       data */
    for (j = 0 ; j < tihm->num_dohm ; j++) {
      dohm = (struct db_dohm *)entry->children[j]->data;
      
      if (dohm->type != dohm_type)
	continue;

      if (unicode_data_len > dohm->len)
	continue;

      if (memmem (&entry->children[j]->data[0x28], dohm->len, unicode_data, unicode_data_len) != 0) {
	ret = tihm->identifier;
	goto found;
      }
    }
  }

 notfound: /* if ret == -1 */
 found:
  free(unicode_data);
  return ret;
}
