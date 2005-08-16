/**
 *   (c) 2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1 album.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>

#include "itunesdbi.h"

int db_album_retrieve (ipoddb_t *photodb, db_alhm_t **alhm_data,
		       tree_node_t **dshm_header, int album, tree_node_t **abhm_header) {
  tree_node_t *temp, *alhm_header;
  db_alhm_t *alhm_data_loc;
  int ret;

  if (photodb == NULL || photodb->type != 1 || (album < 0 && abhm_header == NULL))
    return -EINVAL;

  if ((ret = db_dshm_retrieve (photodb, &temp, 0x2)) < 0)
    return ret;

  alhm_header = temp->children[0];

  alhm_data_loc = (db_alhm_t *)temp->children[0]->data;

  /* Store the pointers if storage was passed in */
  if (alhm_data != NULL)
    *alhm_data = alhm_data_loc;

  if (dshm_header != NULL)
    *dshm_header = temp;

  if (abhm_header != NULL) {
    if (album > (alhm_data_loc->list_entries - 1))
      return -EINVAL;

    *abhm_header = temp->children[album + 1];
  }

  return 0;
}

int db_album_number (ipoddb_t *photodb) {
  db_alhm_t *alhm_data;

  int ret;

  if ((ret = db_album_retrieve (photodb, &alhm_data, NULL, 0, NULL)) < 0)
    return ret;

  return alhm_data->list_entries;
}

struct abhm {
  char *name;
  int num;
  size_t name_len;
};

int db_album_list (ipoddb_t *photodb, db_list_t **head) {
  tree_node_t *dshm_header, *abhm_header;
  tree_node_t *dohm_header = NULL;
  struct abhm *abhm;
  db_alhm_t *alhm_data;
  int i, ret;
  struct string_header_12 *string_header;

  if (head == NULL)
    return -EINVAL;

  db_log (photodb, 0, "db_album_list: entering...\n");

  *head = NULL;

  if ((ret = db_album_retrieve (photodb, &alhm_data, &dshm_header, 0, NULL)) < 0)
    return ret;

  for ( i = dshm_header->num_children-1 ; i > 0 ; i--) {
    abhm_header = dshm_header->children[i];

    dohm_header = abhm_header->children[0];

    string_header = (struct string_header_12 *)&(abhm_header->children[0]->data[0x18]);
    
    abhm     = calloc (1, sizeof (struct pyhm));
    abhm->num = i - 1;

    abhm->name     = calloc (string_header->string_length + 1, 1);
    abhm->name_len = string_header->string_length;
    memcpy (abhm->name, &dohm_header->data[0x24], string_header->string_length);

    *head = db_list_prepend (*head, abhm);
  }

  db_log (photodb, 0, "db_album_list: complete\n");

  return 0;
}

void db_album_list_free (db_list_t **head) {
  db_playlist_list_free (head);
}

int db_album_create (ipoddb_t *photodb, u_int8_t *name) {
  tree_node_t *new_abhm, *new_dohm, *dshm_header;
  db_alhm_t *alhm_data;
  struct string_header_12 *string_header;
  int ret;

  if (name == NULL)
    return -EINVAL;

  db_log (photodb, 0, "db_album_create: entering...\n");

  if ((ret = db_album_retrieve (photodb, &alhm_data, &dshm_header, 0, NULL)) < 0)
    return ret;

  if ((ret = db_abhm_create (&new_abhm)) < 0)
    return ret;

  /* create the title entry for the new playlist */
  if ((ret = db_dohm_create_generic (&new_dohm, 0x18 + 12 + strlen(name), 1)) < 0)
    return ret;

  string_header = (struct string_header_12 *)&(new_dohm->data[0x18]);
  string_header->string_length = strlen(name);
  string_header->format = 0x1;

  new_dohm->string_header_size = 12;

  memcpy (&new_dohm->data[0x24], name, strlen(name));

  db_abhm_dohm_attach (new_abhm, new_dohm);

  /* this MUST be done last to avoid adding the sizes it's children twice */
  db_attach (dshm_header, new_abhm);

  db_log (photodb, 0, "db_album_create: complete\n");

  return alhm_data->list_entries++;
}

int db_album_image_add (ipoddb_t *photodb, int album, int image_id) {
  tree_node_t *abhm_header, *dshm_header, *aihm_header;

  db_alhm_t *alhm_data;

  int entry_num, ret;

  db_log (photodb, 0, "db_album_image_add: entering...\n");

  /* make sure the tihm exists in the database before continuing */
  if ((ret = db_iihm_retrieve (photodb, NULL, NULL, image_id)) < 0)
    return ret;
    
  if ((ret = db_album_retrieve (photodb, &alhm_data, &dshm_header, album, &abhm_header)) < 0)
    return ret;

  /* check if the reference already exists in this playlist */
  entry_num = db_aihm_search (abhm_header, image_id);

  if (entry_num == -1) {
    db_aihm_create (&aihm_header, image_id);
    db_abhm_aihm_attach (abhm_header, aihm_header);
  }

  db_log (photodb, 0, "db_album_image_add: complete\n");

  return 0;
}

int db_album_image_remove (ipoddb_t *photodb, int album, int image_id) {
  tree_node_t *abhm_header, *dshm_header, *aihm_header;

  db_alhm_t *alhm_data;
  struct db_abhm *abhm_data;

  int entry_num, ret;

  if (photodb == NULL || album < 0)
    return -EINVAL;

  db_log (photodb, 0, "db_playlist_tihm_remove: entering...\n");

  if ((ret = db_album_retrieve (photodb, &alhm_data, &dshm_header, album, &abhm_header)) < 0)
    return ret;

  abhm_data   = (struct db_abhm *) abhm_header->data;

  /* search and destroy */
  entry_num = db_aihm_search (abhm_header, image_id);

  if (entry_num > -1) {
    db_detach (abhm_header, entry_num, &aihm_header);
    
    db_free_tree (aihm_header);
    
    abhm_data->num_aihm -= 1;
  }

  db_log (photodb, 0, "db_album_image_remove: complete\n");

  return 0;
}
