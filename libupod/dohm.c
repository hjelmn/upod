/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 dohm.c
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

#define DOHM_HEADER_SIZE   0x018

#define DOHM_EQ_SIZE       0x03a
#define DOHM_PIHM_SIZE     0x02c
#define DOHM_ITUNES_SIZE   0x288

int db_dohm_retrieve (tree_node_t *tihm_header, tree_node_t **dohm_header,
		      int dohm_type) {
  int i;
  struct db_dohm *dohm_data;

  if (dohm_header == NULL)
    return -EINVAL;
  
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

dohm_t *dohm_create (tihm_t *tihm, int data_type) {
  dohm_t *dohm;
  int i;

  if (tihm == NULL)
    return NULL;

  /* Do not allow more than one dohm entry with the same type */
  if (data_type > 0 && data_type != IPOD_PATH)
    for (i = 0 ; i < tihm->num_dohm ; i++)
      if (tihm->dohms[i].type == data_type)
	return NULL;

  if (tihm->num_dohm++ == 0)
    tihm->dohms = (dohm_t *) calloc(1, sizeof(dohm_t));
  else
    tihm->dohms = realloc(tihm->dohms, tihm->num_dohm * sizeof(dohm_t));
  
  if (data_type == IPOD_TITLE) {
    for (i = tihm->num_dohm - 1 ; i > 0 ; i--)
      memmove (&tihm->dohms[i], &tihm->dohms[i-1], sizeof (dohm_t));

    dohm = tihm->dohms;
  } else
    dohm = &(tihm->dohms[tihm->num_dohm - 1]);

  memset (dohm, 0, sizeof(dohm_t));
  dohm->type = data_type;

  return dohm;
}

int dohm_add (tihm_t *tihm, char *data, int data_len, char *encoding, int data_type) {
  dohm_t *dohm;

  if (data_len <= 0 || data_type < 1)
    return -EINVAL;

  if ((dohm = dohm_create (tihm, data_type)) == NULL)
    return -1;

  to_utf8 (&(dohm->data), data, data_len, encoding);

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

int db_dohm_create_generic (tree_node_t **entry, size_t size, int type) {
  struct db_dohm *dohm_data;
  int ret;

  if (size < DOHM_HEADER_SIZE)
    return -EINVAL;

  if ((ret = db_node_allocate (entry, DOHM, DOHM_HEADER_SIZE, size)) < 0)
    return ret;

  (*entry)->data = realloc ((*entry)->data, size);
  memset (&((*entry)->data[DOHM_HEADER_SIZE]), 0, size - DOHM_HEADER_SIZE);
  (*entry)->data_size = size;

  dohm_data = (struct db_dohm *)(*entry)->data;
  dohm_data->type        = type;

  return 0;
}

int db_dohm_itunes_create (tree_node_t **entry) {
  struct db_wierd_dohm *wierd_dohm_data;

  if (entry == NULL)
    return -EINVAL;

  db_dohm_create_generic (entry, DOHM_ITUNES_SIZE, 0x64);

  wierd_dohm_data = (struct db_wierd_dohm *)(*entry)->data;

  /* i dont know what these two are about, but they need to be set like this */
  wierd_dohm_data->unk6 = 0x00010000;
  wierd_dohm_data->unk7 = 0x00000004;

  return 0;
}
						
int db_dohm_itunes_show (tree_node_t *entry, int column_id, int column_width) {
  struct db_wierd_dohm *wierd_dohm_data;
  int num_shown;

  if (entry == NULL)
    return -EINVAL;

  wierd_dohm_data = (struct db_wierd_dohm *)entry->data;
  
  /* This value stores the number of columns show */
  num_shown = wierd_dohm_data->shows[0].unk10[0];

  if (num_shown == 37)
    /* No more columns can be shown */
    return -EINVAL;

  wierd_dohm_data->shows[0].unk10[0] += 1;

  wierd_dohm_data->shows[num_shown].show = column_id | (column_width << 16);

  return 0;
}

int db_dohm_itunes_hide (tree_node_t *entry, int column_id) {
  struct db_wierd_dohm *wierd_dohm_data;
  int num_shown;
  int i, j;

  if (entry == NULL)
    return -EINVAL;

  wierd_dohm_data = (struct db_wierd_dohm *)entry->data;
  num_shown = wierd_dohm_data->shows[0].unk10[0];

  if (num_shown == 0)
    return -EINVAL;

  for (i = 0 ; i < num_shown ; i++) {
    if ((wierd_dohm_data->shows[i].show & 0xff) == column_id)
      break;
  }

  if (i == num_shown)
    return -EINVAL;

  for (j = i + 1 ; j < num_shown ; j++)
    wierd_dohm_data->shows[j-1] = wierd_dohm_data->shows[j];

  wierd_dohm_data->shows[0].unk10[0] -= 1;

  return 0;
}

int db_dohm_create_pihm (tree_node_t **entry, int order) {
  int *iptr;

  db_dohm_create_generic (entry, DOHM_PIHM_SIZE, 0x64);
  
  iptr = (int *)&((*entry)->data[0x18]);
  iptr[0] = order;

  return 0;
}

int db_dohm_create (tree_node_t **entry, dohm_t dohm, int string_header_size, int flags) {
  int entry_size;
  u_int16_t *unicode_data;
  size_t unicode_length;

  if (!(flags & FLAG_UTF8)) {
    if ((dohm.type == IPOD_PATH) && (flags & FLAG_UNICODE_HACK) )
      to_unicode_hack (&unicode_data, &unicode_length, dohm.data, strlen (dohm.data), "UTF-8");
    else
      to_unicode (&unicode_data, &unicode_length, dohm.data, strlen (dohm.data), "UTF-8");
  } else {
    unicode_data   = (u_int16_t *)dohm.data;
    unicode_length = strlen (dohm.data);
  }

  entry_size   = DOHM_HEADER_SIZE + string_header_size + unicode_length;

  db_dohm_create_generic (entry, entry_size, dohm.type);

  if (string_header_size == 12) {
    struct string_header_12 *string_header;
    
    string_header = (struct string_header_12 *)&((*entry)->data[DOHM_HEADER_SIZE]);

    string_header->string_length = unicode_length;

    if (!(flags & FLAG_UTF8))
      string_header->format = 0x00000002;
    else
      string_header->format = 0x00000001;
  } else {
    struct string_header_16 *string_header;
    
    string_header = (struct string_header_16 *)&((*entry)->data[DOHM_HEADER_SIZE]);

    string_header->string_length = unicode_length;
    string_header->unk0 = 0x00000001;

    if (!(flags & FLAG_UTF8))
      string_header->format = 0x00000000;
    else
      string_header->format = 0x00000001;
  }

  (*entry)->string_header_size = string_header_size;

  memcpy(&(*entry)->data[DOHM_HEADER_SIZE + string_header_size], unicode_data,
	 unicode_length);
  
  return 0;
}

/*
  db_dohm_create_eq:

  Creates a generic eq dohm for use with a song entry.

  Returns:
   < 0 if an error occured
     0 if successful
*/
int db_dohm_create_eq (tree_node_t **entry, u_int8_t eq) {
  dohm_t dohm;
  char ceq[9] = "\x23\x21\x23\x31\x30\x00\x23\x21\x23";

  ceq[5] = eq;
  dohm.data = ceq;
  dohm.type = IPOD_EQ;
  
  db_dohm_create (entry, dohm, 16, 0);

  return 0;
}

dohm_t *db_dohm_fill (tree_node_t *entry) {
  dohm_t *dohms;
  struct db_dohm *dohm_data;
  tree_node_t *dohm_header;
  int i, *iptr;
  int string_format;
  int string_length;
  u_int8_t *string_start;

  dohms = (dohm_t *) calloc (entry->num_children, sizeof(dohm_t));

  iptr = (int *)entry->data;

  for (i = 0 ; i < entry->num_children ; i++) {
    dohm_header = entry->children[i];
    dohm_data   = (struct db_dohm *)dohm_header->data;

    if (dohm_header->string_header_size == 12) {
      struct string_header_12 *string_header = (struct string_header_12 *)&(dohm_header->data[0x18]);

      string_length = string_header->string_length;
      string_format = string_header->format;

      string_start  = &(dohm_header->data[0x24]);
    } else if (dohm_header->string_header_size == 16) {
      struct string_header_16 *string_header = (struct string_header_16 *)&(dohm_header->data[0x18]);

      string_length = string_header->string_length;
      string_format = string_header->format;

      string_start  = &(dohm_header->data[0x28]);
    } else {
      dohms[i].type = -1;

      continue;
    }

    dohms[i].type = dohm_data->type;

    to_utf8 (&(dohms[i].data), string_start, string_length, (string_format == 1) ? "UTF-8" : "UTF-16BE");
  }

  return dohms;
}

void dohm_free (dohm_t *dohm, int num_dohms) {
  int i;

  if (dohm == NULL)
    return;

  for (i = 0 ; i < num_dohms ; i++)
    if (dohm[i].data != NULL)
      free(dohm[i].data);

  free(dohm);
}

/**
  db_dohm_tihm_modify:

   Modifies (or adds) a dohm in (to) a song entry.

  Arguments:
   ipoddb_t *itunesdb - opened itunesdb
   int         tihm_num - song entry to be modified
   dohm_t     *dohm     - structure containing information to be changed

  Returns:
   < 0 on error
     0 on success
**/
int db_dohm_tihm_modify (ipoddb_t *itunesdb, int tihm_num, dohm_t *dohm) {
  tree_node_t *tihm_header, *dohm_header;

  int entry_num, ret;

  if ((ret = db_tihm_retrieve (itunesdb, &tihm_header, NULL, tihm_num)) < 0)
    return ret;

  if ((entry_num = db_dohm_retrieve (tihm_header, &dohm_header, dohm->type)) == 0) {
    /* the tree is never deep, so this ends up costing theta(1) extra */
    db_detach (tihm_header, entry_num, &dohm_header);
    
    db_free_tree (dohm_header);
  }

  db_dohm_create (&dohm_header, *dohm, 16, 0);
  db_attach (tihm_header, dohm_header);

  return 0;
}
