/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.2a ipod.h
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

#ifndef _ITUNESDB_H
#define _ITUNESDB_H

#include <sys/types.h>

struct _glist {
  struct _glist *next, *prev;
  void *data;
};

typedef struct _glist GList;

enum dohm_types_t {
  IPOD_TITLE=1,
  IPOD_PATH,
  IPOD_ALBUM,
  IPOD_ARTIST,
  IPOD_GENRE,
  IPOD_TYPE, IPOD_EQ,
  IPOD_COMMENT
};

enum show_entries {
  SHOW_PLAYING    = 0x01,
  SHOW_TITLE      = 0x02,
  SHOW_ALBUM      = 0x03,
  SHOW_ARTIST     = 0x04,
  SHOW_BITRATE    = 0x05,
  SHOW_SAMPLERATE = 0x06,
  SHOW_YEAR       = 0x07,
  SHOW_GENRE      = 0x08,
  SHOW_KIND       = 0x09,
  SHOW_MOD_DATE   = 0x0a,
  SHOW_TRACK_NUM  = 0x0b,
  SHOW_SIZE       = 0x0c,
  SHOW_TIME       = 0x0d,
  SHOW_COMMENT    = 0x0e,
  SHOW_DATE_ADDED = 0x10,
  SHOW_EQUILIZER  = 0x11,
  SHOW_COMPOSER   = 0x12,
  SHOW_PLAY_COUNT = 0x14,
  SHOW_LAST_PLAYED= 0x15,
  SHOW_DISK_NUMBER= 0x16,
  SHOW_STARS      = 0x17
};

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

typedef struct tree itunesdb_t;
typedef struct tree_node tree_node_t;
 
typedef struct dohm {
  u_int32_t type;
  u_int32_t size;

  /* unicode string */
  u_int16_t *data;
} dohm_t;

typedef dohm_t mhod_t;

typedef struct tihm {
  int num;

  u_int32_t type;
  u_int32_t bitrate;
  u_int32_t samplerate;
  u_int32_t length;
  u_int32_t size;
  u_int32_t track;

  /* in thousants of seconds */
  u_int32_t time;
  u_int32_t start_time;
  u_int32_t stop_time;

  u_int32_t volume_adjustment;
  u_int32_t times_played;
  u_int32_t stars;
  u_int32_t played_date;
  u_int32_t mod_date;
  u_int32_t creation_date;

  int num_dohm;

  dohm_t *dohms;
} tihm_t;

typedef tihm_t mhit_t;

typedef struct pyhm {
  int num;
  char *name;
} pyhm_t;

typedef pyhm_t mhyp_t;

/* all strings passed to the below functions are ascii not unicode */

/* libupod/db.c */
int    db_create(itunesdb_t *itunesdb, char *db_name, int name_len);
int    db_load  (itunesdb_t *itunesdb, char *path);
int    db_write (itunesdb_t itunesdb, char *path);
int    db_remove(itunesdb_t *itunesdb, u_int32_t tihm_num);
int    db_add   (itunesdb_t *itunesdb, char *filename, char *path, int path_len, int stars);
int    db_dohm_tihm_modify (itunesdb_t *itunesdb, int tihm_num, dohm_t *dohm);

/* make sure all the values contained in the tihm are correct, there is currently no
   checks so you could screw up a working song entry */
int    db_song_modify (itunesdb_t *itunesdb, int tihm_num, tihm_t *tihm);

/* returns the tihm number of the first match to data of dohm_type */
int    db_lookup_tihm (itunesdb_t *itunesdb, int dohm_type, char *data,
		       int data_len);
/* returns the playlist number of first match */
int    db_lookup_playlist (itunesdb_t *itunesdb, char *data, int data_len);

/* eq is an integer specifier from TunesEQPresets */
int    db_modify_eq(itunesdb_t *itunesdb, u_int32_t tihm_num, int eq);
/* volume adjustment is an integer between -100 and +100 */
int    db_modify_volume_adjustment (itunesdb_t *itunesdb, u_int32_t tihm_num,
				    int volume_adjustment) ;
/* start and stop times are in seconds */
int    db_modify_start_stop_time   (itunesdb_t *itunesdb, u_int32_t tihm_num,
				    int start_time, int stop_time);


int db_playlist_number      (itunesdb_t *itunesdb);
int db_playlist_create      (itunesdb_t *itunesdb, char *name, int name_len);
int db_playlist_rename      (itunesdb_t *itunesdb, int playlist, char *name);
int db_playlist_delete      (itunesdb_t *itunesdb, int playlist);
int db_playlist_tihm_add    (itunesdb_t *itunesdb, int playlist, int tihm_num);
int db_playlist_tihm_remove (itunesdb_t *itunesdb, int playlist, int tihm_num);
int db_playlist_clear       (itunesdb_t *itunesdb, int playlist);
int db_playlist_fill        (itunesdb_t *itunesdb, int playlist);
int db_playlist_remove_all  (itunesdb_t *itunesdb, int tihm_num);

/* this has to deal with the view in itunes, you can use them if you
   wish */
int db_playlist_column_show (itunesdb_t *itunesdb, int playlist, int column,
			     u_int16_t width);
int db_playlist_column_hide (itunesdb_t *itunesdb, int playlist, int column);
int db_playlist_column_move (itunesdb_t *itunesdb, int playlist, int cola,
			     int pos);


/* returns a list of the playlists on the itunesdb. The data field is of
   type struct pyhm */
GList *db_playlist_list        (itunesdb_t *itunesdb);
GList *db_song_list            (itunesdb_t *itunesdb);
int    db_playlist_list_songs  (itunesdb_t *itunesdb, int playlist,
				int **list);


/* functions for cleaning up memory */
void   tihm_free             (tihm_t *tihm);
void   db_free               (itunesdb_t *itunesdb);
void   db_song_list_free     (GList *head);
void   db_playlist_list_free (GList *head);

#endif