/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.1a playlist.c
 *
 *   Functions for managing playlists on the iPod.
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

#include <errno.h>

#define PYHM             0x7079686d
#define PYHM_HEADER_SIZE 0x6c

int db_playlist_retrieve_header (struct tree iTunesDB, struct tree_node **entry,
				 struct tree_node **dshm) {
  struct tree_node **master, *root;

  if (entry == NULL) return -1;

  root = iTunesDB.tree_root;

  if (root == NULL) return -1;

  for (master = &(root->children[root->num_children-1]) ;
       !strstr((*master)->data, "dshm") ; master--);

  *entry = (*master)->children[0];

  if (dshm != NULL) *dshm = *master;

  return 0;
}

int db_return_num_playlists (ipod_t *ipod) {
  struct tree_node *entry;
  struct db_plhm *plhm;

  if (ipod == NULL) return -1;

  if (db_playlist_retrieve_header (ipod->iTunesDB, &entry, NULL) < 0)
    return -1;

  plhm = (struct db_plhm *) entry->data;

  return plhm->num_pyhm;
}

GList *db_return_playlist_list (ipod_t *ipod) {
  struct tree_node *plhm_header, *dshm_header, *pyhm_header, *dohm_header;
  struct pyhm *pyhm;
  struct db_plhm *plhm_data;
  struct db_dohm *dohm_data;

  GList *head, **current, *prev = NULL;

  int i;

  if (db_playlist_retrieve_header (ipod->iTunesDB, &plhm_header, &dshm_header) < 0)
    return NULL;

  plhm_data = (struct db_plhm *) plhm_header->data;

  current = &head;
  for ( i = 1 ; i <= plhm_data->num_pyhm ; i++ ) {
    pyhm_header = dshm_header->children[i];
    dohm_header = pyhm_header->children[1];
    dohm_data   = (struct db_dohm *)dohm_header->data;

    *current = malloc (sizeof (GList));
    pyhm     = malloc (sizeof (struct pyhm));
    memset (pyhm, 0, sizeof (struct pyhm));

    pyhm->num = i - 1;
    pyhm->name= malloc (dohm_data->len / 2);
    memset (pyhm->name, 0, dohm_data->len / 2);

    unicode_to_char (pyhm->name, &dohm_header->data[0x28], dohm_data->len);

    (*current)->data = (void *)pyhm;
    
    (*current)->prev = prev;
    prev = *current;
    current = &(*current)->next;
  }

  return head;
}

void db_playlist_list_free (GList *head) {
  struct pyhm *pyhm;

  while (head) {
    GList *tmp = head->next;

    if (head->data) {
      pyhm = (struct pyhm *)head->data;

      if (pyhm->name)
	free (pyhm->name);

      free (head->data);
    }
    
    free (head);
    head = tmp;
  }
}

int db_playlist_list_songs (ipod_t *ipod, int playlist, int **list) {
  struct tree_node *plhm_header, *dshm_header, *pyhm_header;

  struct db_plhm *plhm_data;
  struct db_pyhm *pyhm_data;
  struct db_pihm *pihm_data;

  int i;


  if (ipod == NULL || list == NULL) return -1;

  if (db_playlist_retrieve_header (ipod->iTunesDB, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist < 0 || playlist >= plhm_data->num_pyhm) return -1;

  pyhm_header = dshm_header->children[playlist + 1];
  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  *list = malloc (sizeof (int) * pyhm_data->num_pihm);

  for (i = 0 ; i < pyhm_data->num_pihm ; i++) {
    pihm_data = (struct db_pihm *)pyhm_header->children[i * 2 + 2]->data;
    (*list)[i] = pihm_data->reference;
  }

  return pyhm_data->num_pihm;
}

static int db_pyhm_create (struct tree_node *entry) {
  struct db_pyhm *pyhm_data;
  if (entry == NULL) return -1;

  memset (entry, 0, sizeof(struct tree_node));

  entry->size = PYHM_HEADER_SIZE;
  entry->data = malloc (PYHM_HEADER_SIZE);
  if (entry->data = NULL) {
    perror ("db_pyhm_create|malloc");
    return -1;
  }

  pyhm_data = (struct db_pyhm *)entry->data;
  pyhm_data->pyhm        = PYHM;
  pyhm_data->header_size = PYHM_HEADER_SIZE;
  pyhm_data->record_size = PYHM_HEADER_SIZE;
}

int db_create_new_playlist (ipod_t *ipod, char *name) {
  struct tree_node *plhm_header, *new_pyhm, *new_dohm;
  struct db_plhm *plhm;
  struct db_dohm *dohm;

  if (name == NULL) return -1;
  if (ipod == NULL) return -1;

  if (db_playlist_retrieve_header (ipod->iTunesDB, &plhm_header, NULL) < 0)
    return -1;

  plhm = (struct db_plhm *) plhm_header->data;

  if (db_pyhm_create (new_pyhm) < 0)
    return -1;

  /* I dont know what this dohm entry does */
  new_dohm = (struct tree_node *) malloc (sizeof (struct tree_node));
  if (new_dohm == NULL) {
    perror ("db_create_new_playlist|malloc");
    return errno;
  }
  if (db_dohm_create_generic (new_dohm, 0x288, 0x000) < 0)
    return -1;

  dohm       = (struct db_dohm *)new_dohm->data;
  dohm->type = 0x64;

  db_attach (new_pyhm, new_dohm);

  /* create the title entry for the new playlist */
  new_dohm = (struct tree_node *) malloc (sizeof (struct tree_node));
  if (new_dohm == NULL) {
    perror ("db_create_new_playlist|malloc");
    return errno;
  }
  if (db_dohm_create_generic (new_dohm, 0x18 + 0x16 + 2 * strlen (name), 0) < 0)
    return -1;

  dohm       = (struct db_dohm *)new_dohm->data;
  dohm->type = IPOD_TITLE;
  dohm->unk2 = 1;
  dohm->len  = 2 * strlen (name);

  char_to_unicode (&new_dohm->data[0x28], name, strlen(name));

  db_attach (new_pyhm, new_dohm);

  /* this MUST be done last to avoid adding the sizes it's children twice */
  db_attach (plhm_header, new_pyhm);

  return plhm->num_pyhm++;
}

int db_remove_playlist (ipod_t *ipod, int playlist) {
  struct tree_node *plhm_header, *dshm_header, *pyhm_header;
  struct db_plhm *plhm_data;

  if (ipod == NULL) return -1;

  if (db_playlist_retrieve_header (ipod->iTunesDB, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *)plhm_header->data;

  /* dont allow the deletion of the master playlist */
  if (playlist < 1 || playlist >= plhm_data->num_pyhm)
    return -1;

  db_detach (dshm_header, playlist + 1, &pyhm_header);
  db_free_tree (pyhm_header);
}

int db_add_to_playlist (ipod_t *ipod, int playlist, int tihm_num) {
  struct tree_node *plhm_header, *pyhm_header, *dshm_header;
  struct tree_node *pihm, *dohm;

  struct db_plhm *plhm_data;
  struct db_pyhm *pyhm_data;

  if (ipod == NULL) return -1;
  
  if (db_tihm_retrieve (ipod->iTunesDB, NULL, NULL, tihm_num) < 0)
    return -1;
    
  if (db_playlist_retrieve_header (ipod->iTunesDB, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  pyhm_header = dshm_header->children[playlist + 1];
  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  /* allocate memory */
  pihm = (struct tree_node *) malloc (sizeof (struct tree_node));
  if (pihm == NULL) {
    perror ("db_add_to_playlist|malloc");
    return -1;
  }

  dohm = (struct tree_node *) malloc (sizeof (struct tree_node));
  if (dohm == NULL) {
    perror ("db_add_to_playlist|malloc");
    return -1;
  }

  db_pihm_create (pihm, tihm_num, tihm_num);
  db_dohm_create_generic (dohm, 0x2c, tihm_num);

  db_attach (pyhm_header, pihm);
  db_attach (pyhm_header, dohm);

  pyhm_data->num_pihm += 1;

  return 0;
}

int db_remove_from_playlist (ipod_t *ipod, int playlist, int tihm_num) {
  struct tree_node *plhm_header, *pyhm_header, *dshm_header;
  struct tree_node *pihm, *dohm;

  struct db_plhm *plhm_data;
  struct db_pyhm *pyhm_data;

  int entry_num;

  if (ipod == NULL) return -1;
  
  if (db_playlist_retrieve_header (ipod->iTunesDB, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  pyhm_header = dshm_header->children[playlist + 1];
  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  /* search and destroy */
  entry_num = db_pihm_search (pyhm_header, tihm_num);

  db_detach (pyhm_header, entry_num, &pihm);
  db_detach (pyhm_header, entry_num, &dohm);

  db_free_tree (pihm);
  db_free_tree (dohm);

  pyhm_data->num_pihm -= 1;

  return 0;
}

int db_playlist_clear (ipod_t *ipod, int playlist) {
  struct tree_node *plhm_header, *pyhm_header, *dshm_header;
  struct tree_node *pihm, *dohm;

  struct db_plhm *plhm_data;
  struct db_pyhm *pyhm_data;

  int entry_num;

  if (ipod == NULL) return -1;
  
  if (db_playlist_retrieve_header (ipod->iTunesDB, &plhm_header, &dshm_header) < 0)
    return -1;

  plhm_data = (struct db_plhm *) plhm_header->data;

  if (playlist >= plhm_data->num_pyhm)
    return -1;

  pyhm_header = dshm_header->children[playlist + 1];
  pyhm_data   = (struct db_pyhm *) pyhm_header->data;

  while (pyhm_data->num_pihm--) {
    db_detach (pyhm_header, 2, &pihm);
    db_detach (pyhm_header, 2, &dohm);

    db_free_tree (pihm);
    db_free_tree (dohm);
  }

  pyhm_data->num_pihm = 0;
}

int db_playlist_fill (ipod_t *ipod, int playlist) {
  struct tree_node *first_dshm, *tlhm_header, *tihm_header;

  struct db_tlhm *tlhm_data;
  struct db_tihm *tihm_data;

  int i;

  if (ipod == NULL || ipod->iTunesDB.tree_root == NULL) return -1;

  db_playlist_clear (ipod, playlist);

  first_dshm = ipod->iTunesDB.tree_root->children[0];
  tlhm_header= first_dshm->children[0];
  tlhm_data  = (struct db_tlhm *)tlhm_header->data;

  for (i = 0 ; i < tlhm_data->num_tihm ; i++) {
    tihm_data = (struct db_tihm *)first_dshm->children[i + 1]->data;
    db_add_to_playlist (ipod, playlist, tihm_data->identifier);
  }

  return 0;
}
