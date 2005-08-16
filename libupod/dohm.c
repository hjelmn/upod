/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.4.0 dohm.c
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

#define DOHM_EQ_SIZE       0x03a
#define DOHM_PIHM_SIZE     0x02c
#define DOHM_ITUNES_SIZE   0x288

/* returns first dohm entry with type dohm_type */
int db_dohm_retrieve (tree_node_t *entry, tree_node_t **dohm_header, int dohm_type) {
  int i;
  int num_dohms;

  struct db_dohm *dohm_data;
  struct db_generic *data;

  if (dohm_header == NULL || entry == NULL || dohm_type < 1)
    return -EINVAL;
  
  data = (struct db_generic *)entry->data;

  /* limit the search if the entry has a num_dohm field */
  if (data->type == TIHM || data->type == PYHM)
    num_dohms = data->num_dohm;
  else
    num_dohms = entry->num_children;

  for (i = 0 ; i < num_dohms ; i++) {
    dohm_data = (struct db_dohm *)entry->children[i]->data;

    if (dohm_data->dohm == DOHM && dohm_data->type == dohm_type) {
      *dohm_header = entry->children[i];
      return i;
    }
  }

  *dohm_header = 0;

  return -1;
}

/* Assumes a non-null string */
int db_dohm_compare (tree_node_t *dohm_header1, tree_node_t *dohm_header2) {
  u_int8_t *utf8_1, *utf8_1p;
  u_int8_t *utf8_2, *utf8_2p;

  size_t length_1;
  size_t length_2;

  size_t shortest_string;

  int cmp = 0;
  int i;
  
  if (dohm_header1 == NULL && dohm_header2 == NULL)
    return 0;
  else if (dohm_header1 == NULL)
    return 1;
  else if (dohm_header2 == NULL)
    return -1;

  db_dohm_get_string (dohm_header1, &utf8_1);
  db_dohm_get_string (dohm_header2, &utf8_2);

  /* Ignore leading "the " in strings */
  if (strlen (utf8_1) > 4 && strncasecmp (utf8_1, "the ", 4) == 0)
    utf8_1p = &utf8_1[4];
  else
    utf8_1p = utf8_1;

  if (strlen (utf8_2) > 4 && strncasecmp (utf8_2, "the ", 4) == 0)
    utf8_2p = &utf8_2[4];
  else
    utf8_2p = utf8_2;


  length_1 = strlen (utf8_1p);
  length_2 = strlen (utf8_2p);

  shortest_string = ((length_1 < length_2) ? length_1 :length_2) + 1;

  for (i = 0 ; !cmp && i < shortest_string ; i++)
    if (tolower (utf8_1p[i]) > tolower (utf8_2p[i]))
      cmp = 1;
    else if (tolower(utf8_1p[i]) < tolower(utf8_2p[i]))
      cmp = -1;

  free (utf8_1);
  free (utf8_2);

  return cmp;
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

  if (tihm->num_dohm++ == 0) {
    tihm->dohms = (dohm_t *) calloc(1, sizeof(dohm_t));
  } else
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

  if (size < DOHM_CELL_SIZE)
    return -EINVAL;

  if ((ret = db_node_allocate (entry, DOHM, DOHM_CELL_SIZE, size)) < 0)
    return ret;

  (*entry)->data = realloc ((*entry)->data, size);
  memset (&((*entry)->data[DOHM_CELL_SIZE]), 0, size - DOHM_CELL_SIZE);
  (*entry)->data_size = size;

  dohm_data = (struct db_dohm *)(*entry)->data;
  dohm_data->type        = type;

  return 0;
}

int db_dohm_index_create (tree_node_t **entry, int sort_by, int num_tracks, u_int32_t tracks[]) {
  struct db_dohm *dohm_data;
  int *iptr;
  int ret;

  if (entry == NULL)
    return -EINVAL;

  if ((ret = db_dohm_create_generic (entry, DOHM_CELL_SIZE + 0x28 + 4 * num_tracks, 0x34)) < 0)
    return ret;

  dohm_data = (struct db_dohm *)(*entry)->data;
  iptr = (int *)&((*entry)->data[0x18]);

  if (sort_by == IPOD_TITLE)
    iptr[0] = 0x00000003;
  else if (sort_by == IPOD_ALBUM)
    iptr[0] = 0x00000004;
  else if (sort_by == IPOD_ARTIST)
    iptr[0] = 0x00000005;
  else if (sort_by == IPOD_GENRE)
    iptr[0] = 0x00000007;
  else if (sort_by == IPOD_COMPOSER)
    iptr[0] = 0x00000012;

  iptr[1] = num_tracks;

  iptr += 10;

  memcpy (iptr, tracks, num_tracks * 4);

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
      to_unicode (&unicode_data, &unicode_length, dohm.data, strlen (dohm.data), "UTF-8", UTF_ENC);
  } else {
    unicode_data   = (u_int16_t *)dohm.data;
    unicode_length = strlen (dohm.data);
  }

  entry_size   = DOHM_CELL_SIZE + string_header_size + unicode_length;

  db_dohm_create_generic (entry, entry_size, dohm.type);

  if (string_header_size == 12) {
    struct string_header_12 *string_header;
    
    string_header = (struct string_header_12 *)&((*entry)->data[DOHM_CELL_SIZE]);

    string_header->string_length = unicode_length;

    if (!(flags & FLAG_UTF8))
      string_header->format = 0x00000002;
    else
      string_header->format = 0x00000001;
  } else {
    struct string_header_16 *string_header;
    
    string_header = (struct string_header_16 *)&((*entry)->data[DOHM_CELL_SIZE]);

    string_header->string_length = unicode_length;
    string_header->unk0 = 0x00000001;

    if (!(flags & FLAG_UTF8))
      string_header->format = 0x00000000;
    else
      string_header->format = 0x00000001;
  }

  (*entry)->string_header_size = string_header_size;

  memcpy(&(*entry)->data[DOHM_CELL_SIZE + string_header_size], unicode_data,
	 unicode_length);
  
  if (!(flags & FLAG_UTF8)) {
    free (unicode_data);
  }

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

int db_dohm_get_string (tree_node_t *dohm_header, u_int8_t **str) {
  struct db_dohm *dohm_data;
  u_int8_t *string_start;
  int string_format;
  int string_length;

  if (dohm_header == NULL || str == NULL)
    return -EINVAL;
  
  dohm_data   = (struct db_dohm *)dohm_header->data;
  
  if (dohm_header->string_header_size == 12) {
    struct string_header_12 *string_header = (struct string_header_12 *)&(dohm_header->data[0x18]);
    
    string_length = string_header->string_length;
    string_format = string_header->format;
    
    string_start  = &(dohm_header->data[0x24]);
  } else if (dohm_header->string_header_size == 16) {
    struct string_header_16 *string_header = (struct string_header_16 *)&(dohm_header->data[0x18]);
    
    string_length = string_header->string_length;
    string_format = (string_header->unk0 == 1) ? 0 : 1;
    
    string_start  = &(dohm_header->data[0x28]);
  } else
    return -1;

  to_utf8 (str, string_start, string_length, (string_format == 1) ? "UTF-8" : UTF_ENC);

  return 0;
}

int db_dohm_fill (tree_node_t *entry, dohm_t **dohms) {
  tree_node_t *dohm_header;
  struct db_dohm *dohm_data;
  int i, *iptr;

  if (entry == NULL || dohms == NULL)
    return -EINVAL;

  *dohms = (dohm_t *) calloc (entry->num_children, sizeof(dohm_t));

  iptr = (int *)entry->data;

  for (i = 0 ; i < entry->num_children ; i++) {
    dohm_header = entry->children[i];
    dohm_data = (struct db_dohm *)dohm_header->data;

    if (db_dohm_get_string (dohm_header, &((*dohms)[i].data)) < 0) {
      (*dohms)[i].type = -1;

      continue;
    }

    (*dohms)[i].type = dohm_data->type;
  }

  return 0;
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
