/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a ipod.h
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

#include <sys/types.h>
#include <glib.h>

enum file_types_t {TITLE=1, PATH, ALBUM, ARTIST, GENRE, TYPE, COMMENT};

struct tree {
  struct tree_node {
    struct tree_node *parent;

    char *data;
    size_t size;

    int num_children;
    struct tree_node **children;
  };

  struct tree_node *tree_root;
};

typedef struct _ipod {
  struct tree iTunesDB;
} ipod_t;

typedef struct dohm {
  u_int32_t type;
  u_int32_t size;

  /* unicode string */
  char *data;
} dohm_t;

typedef struct tihm {
  int num;

  u_int32_t type;
  u_int32_t encoding;
  u_int32_t bitrate;
  u_int32_t samplerate;
  u_int32_t length;
  u_int32_t size;
  u_int32_t mod_date;

  /* in thousants of seconds */
  u_int32_t time;

  int num_dohm;

  dohm_t *dohms;
} tihm_t;

/* libupod/db.c */
int    db_load  (ipod_t *ipod, char *path);
void   db_free  (ipod_t *ipod);
int    db_write (ipod_t ipod, char *path);

int    db_remove(ipod_t *ipod, u_int32_t tihm_num);
int    db_hide  (ipod_t *ipod, u_int32_t tihm_num);

int    db_add   (ipod_t *ipod, char *filename, char *path);
int    db_unhide(ipod_t *ipod, u_int32_t tihm_num);

GList *db_song_list (ipod_t ipod);
void   db_song_list_free (GList *head);
