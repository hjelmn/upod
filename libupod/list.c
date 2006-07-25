/**
 *   (c) 2002-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v1.0.0 list.c
 *
 *   Simple GList like list implementation
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

db_list_t *db_list_first (db_list_t *p) {
  db_list_t *x;

  for (x = p ; x && x->prev ; x = x->prev);

  return x;
}

db_list_t *db_list_prepend (db_list_t *p, void *d) {
  db_list_t *x, *y;

  x = calloc (1, sizeof (db_list_t));
  x->data = d;

  y = db_list_first (p);

  x->next = y;

  if (y)
    y->prev = x;

  return x;
}

db_list_t *db_list_append (db_list_t *p, void *d) {
  db_list_t *x, *y;

  x = calloc (1, sizeof (db_list_t));
  x->data = d;

  for (y = db_list_first (p) ; y && y->next ; y = db_list_next (y));

  x->prev = y;

  if (y) {
    y->next = x;
    return p;
  }

  return x;
}

db_list_t *db_list_next (db_list_t *p) {
  if (p == NULL)
    return p;

  return p->next;
}

void db_list_free (db_list_t *p) {
  db_list_t *tmp;

  p = db_list_first (p);

  while (p) {
    tmp = p->next;
    free (p);
    p = tmp;
  }
}
