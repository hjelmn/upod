/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a dohm.c
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

#define DOHM               0x646f686d
#define DOHM_HEADER_SIZE   0x18

/* creates a new dohm and appends it to the dohm list in the tihm
   structure */
dohm_t *dohm_create (tihm_t *tihm) {
  dohm_t *dohm;

  if (tihm == NULL)
    return NULL;

  if (tihm->num_dohm++ == 0)
    tihm->dohms = (dohm_t *)malloc(sizeof(dohm_t));
  else
    tihm->dohms = realloc(tihm->dohms, tihm->num_dohm * sizeof(dohm_t));
  
  dohm = &(tihm->dohms[tihm->num_dohm - 1]);
  memset (dohm, 0, sizeof(dohm_t));

  return dohm;
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

int db_dohm_create_generic (struct tree_node *entry, size_t size, int junk) {
  int *iptr;
  
  entry->size         = size;
  entry->data         = malloc(size);
  entry->num_children = 0;
  entry->children     = NULL;

  iptr = (int *)entry->data;

  iptr[0] = DOHM;
  iptr[1] = DOHM_HEADER_SIZE;
  iptr[2] = size;
  iptr[3] = 0x64;
  iptr[6] = junk + 1;

  return 0;
}

dohm_t *db_dohm_fill (struct tree_node *entry) {
  dohm_t *dohms;
  struct tree_node **master;
  int i, *iptr;

  dohms = (dohm_t *) malloc (entry->num_children * sizeof(dohm_t));
  memset(dohms, 0, entry->num_children * sizeof(dohm_t));

  master = entry->children;

  for (i = 0 ; i < entry->num_children ; i++) {
    iptr = (int *)master[i]->data;

    dohms[i].type = iptr[3];
    dohms[i].size = iptr[7];
    dohms[i].data = (char *) malloc (dohms[i].size);
    memcpy(dohms[i].data, &iptr[10], dohms[i].size);
  }

  return dohms;
}

void dohm_free (dohm_t *dohm, int num_dohms) {
  int i;

  for (i = 0 ; i < num_dohms ; i++)
    free(dohm[i].data);

  free(dohm);
}
