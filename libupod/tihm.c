/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a tihm.c
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
#include "hexdump.c"

#include <stdlib.h>
#include <stdio.h>

#define TIHM               0x7469686d
#define TIHM_HEADER_SIZE   0x9c

#define DOHM               0x646f686d
#define DOHM_HEADER_SIZE   0x18

#define STRING_HEADER_SIZE 0x10

int db_tihm_search (struct tree_node *entry, u_int32_t tihm_num) {
  int i;

  for ( i = 1 ; i < entry->num_children ; i++ )
    if (((int *)(entry->children[i]->data))[4] == tihm_num)
      return i;

  return -1;
}

tihm_t *tihm_create (tihm_t *tihm, char *filename, char *path, int num) {
  dohm_t *dohm;

  if (strstr(filename, ".mp3") ||
      strstr(filename, ".MP3") )
      mp3_fill_tihm (filename, tihm);

  dohm       = dohm_create(tihm);
  dohm->type = IPOD_PATH;
  dohm->size = 2 * strlen (path);
  dohm->data = calloc (1, dohm->size);
  char_to_unicode (dohm->data, path, strlen(path));

  return tihm;
}

void tihm_destroy (tihm_t *tihm) {
  if (tihm == NULL) return;

  while (tihm->num_dohm--)
    free(tihm->dohms[tihm->num_dohm].data);

  free(tihm->dohms);
}

static int db_dohm_create (struct tree_node *parent, struct tree_node *entry, dohm_t dohm){
  int *iptr;

  entry->parent = parent;
  entry->size   = DOHM_HEADER_SIZE + STRING_HEADER_SIZE + dohm.size;
  entry->data   = calloc (1, entry->size);

  iptr = (int *)entry->data;

  iptr[0]                      = DOHM;
  iptr[1]                      = DOHM_HEADER_SIZE;
  iptr[2]                      = entry->size;
  iptr[3]                      = dohm.type;
  iptr[DOHM_HEADER_SIZE/4]     = 1;
  iptr[DOHM_HEADER_SIZE/4 + 1] = dohm.size;

  memcpy(&entry->data[DOHM_HEADER_SIZE + STRING_HEADER_SIZE], dohm.data, dohm.size);
  
  return entry->size;
}

int db_tihm_create (struct tree_node *entry, char *filename, char *path) {
  tihm_t tihm;
  int tihm_num = ((int *)(entry->parent->children[entry->parent->num_children - 1]->data))[4] + 2;
  int *iptr;

  int size;

  int i;

  tihm_create(&tihm, filename, path, tihm_num);

  entry->size = TIHM_HEADER_SIZE;
  entry->data = calloc (1, TIHM_HEADER_SIZE);
  memset (entry->data, 0, TIHM_HEADER_SIZE);
  
  iptr = (int *)entry->data;
  iptr[0] = TIHM;
  iptr[1] = TIHM_HEADER_SIZE;
  iptr[3] = tihm.num_dohm;
  iptr[4] = tihm_num;
  iptr[5] = 0x1;
  iptr[7] = 0x100;
  iptr[8] = time(NULL);//tihm.mod_date;
  iptr[9] = tihm.size;
  iptr[10]= tihm.time;
  iptr[11]= 0x1;
  iptr[14]= 0x80;
  iptr[15]= tihm.samplerate << 16;
  /* there may be other values wich should be set but i dont know
     which */

  entry->num_children = 0;
  entry->children = (struct tree_node **)malloc(sizeof(struct tree_node *));

  size = TIHM_HEADER_SIZE;

  for (i = 0 ; i < tihm.num_dohm ; i++) {
    entry->num_children++;
    entry->children = realloc(entry->children,
			      entry->num_children * sizeof(struct tree_node *));
    entry->children[entry->num_children-1] = calloc(sizeof(struct tree_node), 1);
    size += db_dohm_create (entry, entry->children[entry->num_children-1], tihm.dohms[i]);
  }

  for (i = 0 ; i < tihm.num_dohm ; i++)
    if (tihm.dohms[i].type == 1) {
      struct tree_node *tmp;

      tmp = entry->children[i];
      entry->children[i] = entry->children[0];
      entry->children[0] = tmp;
    }
    

  iptr[2] = size;
  tihm_destroy (&tihm);

  return tihm_num;
}

tihm_t *db_tihm_fill (struct tree_node *entry) {
  int *iptr = (int *)entry->data;
  struct db_tihm *dbtihm = (struct dbtihm *)entry->data;
  tihm_t *tihm;

  tihm = (tihm_t *) malloc (sizeof(tihm_t));
  memset (tihm, 0, sizeof(tihm_t));
  
  tihm->num_dohm  = dbtihm->num_dohm;
  tihm->num       = dbtihm->identifier;

  tihm->size      = dbtihm->file_size;
  tihm->time      = dbtihm->duration;
  tihm->samplerate= dbtihm->sample_rate >> 16;
  tihm->encoding  = dbtihm->encoding;
  tihm->type      = dbtihm->type;

  tihm->dohms     = db_dohm_fill (entry);

  return tihm;
}

void tihm_free (tihm_t *tihm) {
  if (tihm == NULL) return;

  dohm_free (tihm->dohms, tihm->num_dohm);

  free(tihm);
}
