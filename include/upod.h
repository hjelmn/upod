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
//#include <glib.h>

#define PATH_MAX 255

enum dohm_types_t {IPOD_TITLE=1, IPOD_PATH, IPOD_ALBUM, IPOD_ARTIST, IPOD_GENRE,
		   IPOD_TYPE, IPOD_EQ, IPOD_COMMENT};

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

  char prefix[PATH_MAX];
  char path[PATH_MAX];
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

/* libupod/ipod.c */
int ipod_open   (ipod_t *ipod, char *path);
int ipod_close  (ipod_t *ipod);

/* mac_path is a volume path starting with : and seperated by : */
int ipod_add    (ipod_t *ipod, char *filename, char *mac_path, int use_apple_path);
int ipod_remove (ipod_t *ipod, char *mac_path, int remove_from_db, int remove_file);

/* src and dst can be either mac or unix (if you use a unix path, it does not
 translate onto the ipod) */
int ipod_move   (ipod_t *ipod, char *src, char *dst);
int ipod_copy   (ipod_t *ipod, char *src, char *dst);

/* libupod/db.c */
int    db_load  (ipod_t *ipod, char *path); /* called by ipod_open */
void   db_free  (ipod_t *ipod); /* called by ipod_close */
int    db_write (ipod_t ipod, char *path); /* called by ipod_close */

int    db_remove(ipod_t *ipod, u_int32_t tihm_num); /* called by ipod_remove */
int    db_hide  (ipod_t *ipod, u_int32_t tihm_num); /* called by db_remove */

int    db_add   (ipod_t *ipod, char *filename, char *path); /*called by ipod_add */
int    db_unhide(ipod_t *ipod, u_int32_t tihm_num); /* called by db_add */

int    db_lookup(ipod_t ipod, int dohm_type, char *data, int data_len);

int    db_modify_eq(ipod_t *ipod, u_int32_t tihm_num, int eq);

typedef struct glist {
  struct glist *next;
  void *data;
} GList;

GList *db_song_list (ipod_t ipod);
void   db_song_list_free (GList *head);
