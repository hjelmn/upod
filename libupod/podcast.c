/**
 *   (c) 2002-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.4.1 podcast.c
 *
 *   Routines for reading/writing the iPod's databases.
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

int db_ispodcast (ipoddb_t *itunesdb, int tihm_num) {
  tree_node_t *entry;
  struct db_tihm *tihm_data;

  if (db_tihm_retrieve (itunesdb, &entry, NULL, tihm_num) < 0)
    return -1;

  tihm_data = (struct db_tihm *)entry->data;

  /* there are several ways to detect a podcast. this is one of them */
  if ((tihm_data->flags2 & 0xff000000) == 0x02000000)
    return 1;
  else
    return 0;
}

/* add a track to podcast playlists (if they don't exist they will be created) */
int db_podcast_add_tihm (ipoddb_t *itunesdb, tihm_t *tihm) {
  tree_node_t *new_pyhm, *new_dohm, *dshm_header, *pyhm_header, *pihm_header, *new_pihm, *temp;
  db_plhm_t *plhm_data;
  struct db_pihm *pihm_data;
  struct db_pyhm *pyhm_data;
  int i, ret;
  int next_order = 0;
  int podcast_group = 0;

  db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: entering...\n");

  /* add track to standard podcast playlist (ds 2) */
  if ((ret = db_playlist_get_podcast (itunesdb, 2, NULL)) < 0)
    if (ret = db_playlist_create_podcast (itunesdb, "Podcasts", 2)) {
      db_log (itunesdb, ret, "libupod/podcast.c/db_podcast_add_tihm: fatal error. could not create podcast playlist.\n");
      return ret;
    }
  
  db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: adding track to playlist %i.%i\n", 2, ret);

  db_playlist_tihm_add (itunesdb, ret, 2, tihm->num);

  if ((ret = db_dshm_retrieve (itunesdb, &temp, 3)) < 0) {
    char *db_name;

    db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: creating podcast playlists...\n");

    db_playlist_get_name (itunesdb, 0, 2, &db_name);

    /* create master podcast playlist (third data section) */
    db_dshm_add (itunesdb, PLHM, 3);
    db_playlist_create (itunesdb, db_name, 3);

    free (db_name);

    db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: complete\n");
  }

  db_playlist_tihm_add (itunesdb, 0, 3, tihm->num);

  /* update podcast list */
  {
    char *podcast_name;
    tree_node_t *pyhm_header;

    if ((ret = db_playlist_get_podcast (itunesdb, 3, &pyhm_header)) < 0) {
      if (ret = db_playlist_create_podcast (itunesdb, "Podcasts", 3)) {
	db_log (itunesdb, ret, "libupod/podcast.c/db_podcast_add_tihm: fatal error. could not create podcast playlist.\n");
	return ret;
      }

      ret = db_playlist_get_podcast (itunesdb, 3, &pyhm_header);
    }

    for (i = 0 ; i < tihm->num_dohm ; i++)
      if (tihm->dohms[i].type == IPOD_ARTIST || tihm->dohms[i].type == IPOD_ALBUM) {
	podcast_name = tihm->dohms[i].data;
	break;
      }


    db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: adding podcast to group %s\n", podcast_name);

    pyhm_data = (struct db_pyhm *)pyhm_header->data;

    if (pyhm_data->num_pihm > 0) {
      for (i = pyhm_data->num_dohm ; i < pyhm_header->num_children ; i++) {
	u_int8_t *temp_str;

	pihm_header = pyhm_header->children[i];
	pihm_data = (struct db_pihm *)pihm_header->data;

	if (pihm_data->pihm != PIHM || !(pihm_data->flag & 0x0000100))
	  continue;

	db_dohm_get_string (pihm_header->children[0], &temp_str);

	if (strcasecmp (temp_str, podcast_name) == 0) {
	  podcast_group = pihm_data->order;
	  db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: found podcast group %i.\n", podcast_group);
	  free (temp_str);
	  break;
	}

	free (temp_str);
      }
      
      pihm_data = (struct db_pihm *)pyhm_header->children[pyhm_header->num_children - 1]->data;

      if (pihm_data->pihm != PIHM)
	 pihm_data = (struct db_pihm *)pyhm_header->children[pyhm_header->num_children - 2]->data;

      next_order = pihm_data->order + 1;
    } else
      next_order = itunesdb->last_entry + 100000;

    if (!podcast_group) {
      db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: group doesn't exist yet. creating...\n");
      
      podcast_group = next_order;

      db_pihm_create_podcast (&new_pihm, podcast_name, next_order++);
      db_attach (pyhm_header, new_pihm);

      pyhm_data->num_pihm++;

      db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: created\n");
    }

    db_pihm_create (&new_pihm, tihm->num, next_order, podcast_group);    
    db_attach (pyhm_header, new_pihm);
    pyhm_data->num_pihm++;
  }
  

  db_log (itunesdb, 0, "libupod/podcast.c/db_podcast_add_tihm: complete\n");

  return 0;
}
