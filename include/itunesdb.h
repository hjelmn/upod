/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0a itunesdb.h
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

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>

#include <glib.h>

#define ITUNESDB "iPod_Control/iTunes/iTunesDB"
#define ARTWORKDB "iPod_Control/Artwork/ArtworkDB"
#define PHOTODB "Photos/Photo Database"

enum dohm_types_t {
  IPOD_TITLE=1,
  IPOD_PATH,
  IPOD_ALBUM,
  IPOD_ARTIST,
  IPOD_GENRE,
  IPOD_TYPE, IPOD_EQ,
  IPOD_COMMENT,
  IPOD_COMPOSER = 14
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

enum itunesdb_flags {
  /* Turn on hack so more unicode names will work on the ipod. Files with non-ascii characters in
     their names will no longer play in iTunes when this flag is set. */
  FLAG_UNICODE_HACK = 0x1,
};

typedef struct _ipoddatabase {
  struct tree_node {
    struct tree_node *parent;

    u_int8_t *data;
    size_t size;

    int num_children;
    struct tree_node **children;

    /* Only affects dohm entries containing unicode string. */
    int string_header_size;
  } *tree_root;

  int log_level;
  FILE *log;
  int flags;
  int last_entry;

  int database_type;

  char *path;
} ipoddb_t;

typedef struct tree_node tree_node_t;

typedef struct _ipod {
  ipoddb_t itunesdb;
  ipoddb_t artworkdb;

  char *dir;
  char *itunesdb_path;

  /* Sysinfo */
  char *board;
  char *model_number;
  char *serial_number;
  char *sw_version;
} ipod_t;
 
typedef struct dohm {
  u_int32_t type;
  size_t size;

  /* unicode string */
  u_int16_t *data;
} dohm_t;

typedef dohm_t mhod_t;

typedef struct inhm {
  u_int32_t file_offset;
  u_int32_t image_size;
  u_int16_t height;
  u_int16_t width;

  u_int32_t num_dohm;
  dohm_t *dohms;
} inhm_t;

typedef struct iihm {
  int identifier;

  int id1;
  int id2;

  u_int32_t num_inhm;
  inhm_t *inhms;
} iihm_t;

typedef struct tihm {
  int num;

  u_int32_t type;
  u_int32_t vbr;
  u_int32_t bitrate;
  u_int32_t samplerate;
  u_int32_t length;
  u_int32_t size;

  u_int32_t track;
  u_int32_t album_tracks;

  u_int32_t disk_num;
  u_int32_t disk_total;

  u_int32_t year;

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
  u_int16_t bpm; /* beats/min */

  /* These three fields only apply to color iPods */
  u_int32_t has_artwork;
  u_int32_t artwork_id1;
  u_int32_t artwork_id2;

  unsigned char *image_data;
  size_t image_size;

  int num_dohm;

  dohm_t *dohms;
} tihm_t;

typedef tihm_t mhit_t;

typedef struct pyhm {
  int num;
  u_int8_t *name;
  size_t name_len;
} pyhm_t;

typedef pyhm_t mhyp_t;

/* itunesdb2/db.c */
int    db_load  (ipoddb_t *ipoddb, char *path, int flags);
int    db_write (ipoddb_t ipoddb, char *path);

/* itunesdb2/create.c */
int    db_create (ipoddb_t *ipoddb, char *db_name, int name_len, int flags);
int    db_photo_create (ipoddb_t *photodb);

/* itunesdb2/song_list.c */
int  db_song_remove(ipoddb_t *itunesdb, u_int32_t tihm_num);
int  db_song_add   (ipoddb_t *itunesdb, ipoddb_t *artworkdb, char *path, u_int8_t *mac_path, size_t mac_path_len, int stars, int show);
int  db_song_dohm_tihm_modify (ipoddb_t *itunesdb, int tihm_num, dohm_t *dohm);
/* eq is an integer specifier from TunesEQPresets */
int  db_song_modify_eq(ipoddb_t *itunesdb, u_int32_t tihm_num, int eq);
int  db_song_list (ipoddb_t *itunesdb, GList **head);
void db_song_list_free (GList **head);
int  db_song_hide (ipoddb_t *itunesdb, u_int32_t tihm_num);
int  db_song_unhide (ipoddb_t *itunesdb, u_int32_t tihm_num);

/* itunesdb2/image_list.c */
int  db_photo_add (ipoddb_t *artworkdb, unsigned char *image_data, size_t image_size, int id1, int id2);
int  db_photo_list (ipoddb_t *artworkdb, GList **head);
void db_photo_list_free (GList **head);

int    db_set_debug (ipoddb_t *itunesdb, int level, FILE *out);

/* make sure all the values contained in the tihm are correct, there is currently no
   checks so you could screw up a working song entry */
int    db_song_modify (ipoddb_t *itunesdb, int tihm_num, tihm_t *tihm);

/* returns the tihm number of the first match to data of dohm_type */
int    db_lookup (ipoddb_t *itunesdb, int dohm_type, char *data, int data_len);
/* returns the playlist number of first match */
int    db_lookup_playlist (ipoddb_t *itunesdb, char *data, int data_len);

int db_playlist_number      (ipoddb_t *itunesdb);
int db_playlist_create      (ipoddb_t *itunesdb, char *name, int name_len);
int db_playlist_rename      (ipoddb_t *itunesdb, int playlist, char *name,
			     int name_len);
int db_playlist_delete      (ipoddb_t *itunesdb, int playlist);
int db_playlist_tihm_add    (ipoddb_t *itunesdb, int playlist, int tihm_num);
int db_playlist_tihm_remove (ipoddb_t *itunesdb, int playlist, int tihm_num);
int db_playlist_clear       (ipoddb_t *itunesdb, int playlist);
int db_playlist_fill        (ipoddb_t *itunesdb, int playlist);
int db_playlist_remove_all  (ipoddb_t *itunesdb, int tihm_num);
int db_playlist_get_name    (ipoddb_t *itunesdb, int playlist,
			     u_int8_t **name);

/* this has to deal with the view in itunes, you can use them if you
   wish */
int db_playlist_column_show (ipoddb_t *itunesdb, int playlist, int column,
			     u_int16_t width);
int db_playlist_column_hide (ipoddb_t *itunesdb, int playlist, int column);
int db_playlist_column_move (ipoddb_t *itunesdb, int playlist, int cola,
			     int pos);
int db_playlist_column_list_shown (ipoddb_t *itunesdb, int playlist, int **list);


/* returns a list of the playlists on the itunesdb. The data field is of
   type struct pyhm */
int db_playlist_list (ipoddb_t *itunesdb, GList **head);
int db_playlist_song_list (ipoddb_t *itunesdb, int playlist, GList **head);

int dohm_add (tihm_t *timh, char *data, int data_len, char *encoding, int data_type);
int dohm_add_path (tihm_t *timh, char *data, int data_len, char *encoding, int data_type, int use_ipod_unicode_hack);

/* functions for cleaning up memory */
void tihm_free (tihm_t *tihm);
void iihm_free (iihm_t *iihm);
void inhm_free (inhm_t *inhm);
void db_free   (ipoddb_t *itunesdb);

void db_playlist_list_free (GList **head);
void db_playlist_song_list_free (GList **head);

#if defined(__cplusplus)
}
#endif

#endif
