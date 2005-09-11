/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.5.0 itunesdb.h
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
  
typedef struct _dblist {
  void *data;
  struct _dblist *prev;
  struct _dblist *next;
} db_list_t;

db_list_t *db_list_first (db_list_t *);
db_list_t *db_list_next (db_list_t *);
db_list_t *db_list_prev (db_list_t *);
db_list_t *db_list_prepend (db_list_t *p, void *d);
db_list_t *db_list_append (db_list_t *p, void *d);

#define ITUNESDB     "iPod_Control/iTunes/iTunesDB"
#define ITUNESSD     "iPod_Control/iTunes/iTunesSD"
#define ARTWORKDB    "iPod_Control/Artwork/ArtworkDB"
#define OTG_PLAYLIST "iPod_Control/iTunes/OTGPlaylist"
#define PHOTODB      "Photos/Photo Database"

enum dohm_types_t {
  IPOD_TITLE=1,
  IPOD_PATH,
  IPOD_ALBUM,
  IPOD_ARTIST,
  IPOD_GENRE,
  IPOD_TYPE,
  IPOD_EQ,
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
  /* Turn on hack so more unicode filenames will work with the ipod.
     Files with non-ascii characters in their names will not play in
     iTunes if this flag is set when they are added. */
  FLAG_UNICODE_HACK = 0x1,

  /*
    When this flag is set uPod uses UTF-8 encoding internally.
    UTF-16 will still be used for playlist names.
    
    *NOTE* If the database is opened with iTunes all strings
    will automatically be converted to UTF-16.
  */
  FLAG_UTF8 = 0x2,

  /* this flag tells libupod to parse the database in place (speeds up loading under ipodlinux -- in theory) */
  FLAG_INPLACE = 0x4,
};

typedef struct _ipoddatabase {
  struct tree_node *tree_root;

  int log_level;
  FILE *log;
  int flags;
  int last_entry;

  char *path;
  int type; /* 0 == iTunesDB, 1 == ArtworkDB */
} ipoddb_t;

typedef struct _ipod {
  ipoddb_t itunesdb;
  ipoddb_t artworkdb;
  ipoddb_t photodb;

  char *path;

  int supports_artwork;

  /* Sysinfo */
  char *board;
  char *model_number;
  char *serial_number;
  char *sw_version;
} ipod_t;
 
typedef struct dohm {
  u_int32_t type;

  /* UTF-8 encoded string */
  u_int8_t *data;
} dohm_t;

typedef dohm_t mhod_t;
typedef dohm_t ipod_data_object_t;

typedef struct inhm {
  u_int32_t file_offset;
  u_int32_t image_size;
  u_int16_t height;
  u_int16_t width;

  u_int32_t num_dohm;
  dohm_t *dohms;
} inhm_t;

typedef inhm_t ipod_thumbnail_t;

typedef struct iihm {
  int identifier;

  u_int64_t id;

  u_int32_t num_inhm;
  inhm_t *inhms;
} iihm_t;

typedef iihm_t ipod_image_t;

struct song_info {
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

  u_int32_t time;       /* In milliseconds */
  u_int32_t start_time; /* In milliseconds */
  u_int32_t stop_time;  /* In milliseconds */

  u_int32_t volume_adjustment;
  u_int32_t times_played; /* From iTunes */
  u_int32_t stars; /* iTunes star rating */
  u_int32_t played_date;
  u_int32_t mod_date;
  u_int32_t creation_date;
  u_int16_t bpm; /* beats/min */

  /* These three fields only apply to color iPods */
  u_int32_t has_artwork;
  u_int64_t artwork_id;

  unsigned char *image_data;
  size_t image_size;

  int num_dohm;

  dohm_t *dohms;
};

typedef struct song_info tihm_t;

typedef tihm_t mhit_t;
typedef tihm_t ipod_track_t;

typedef struct pyhm {
  int num;
  u_int8_t *name;
  size_t name_len;
} pyhm_t;

typedef pyhm_t mhyp_t;
typedef pyhm_t ipod_playlist_t;

/* itunesdb2/db.c */
int    db_load  (ipoddb_t *ipoddb, char *path, int flags);
int    db_write (ipoddb_t ipoddb, char *path);

/* itunesdb2/itunessd.c */
int    sd_load (ipoddb_t *ipodsd, char *path, int flags);
int    sd_create (ipoddb_t *ipodsd, u_int8_t *path, int flags);

int    sd_write (ipoddb_t ipodsd, char *path);
int    sd_song_add (ipoddb_t *ipodsd, char *ipod_path, int start, int stop, int volume);

/* itunesdb2/create.c */
int    db_create (ipoddb_t *ipoddb, u_int8_t *db_name, u_int8_t *path, int flags);
int    db_photo_create (ipoddb_t *photodb, u_int8_t *path);

/* itunesdb2/song_list.c */
int  db_song_remove(ipoddb_t *itunesdb, u_int32_t tihm_num);
int  db_song_add   (ipoddb_t *itunesdb, ipoddb_t *artworkdb, char *path, char *mac_path, int stars, int show);
int  db_song_dohm_tihm_modify (ipoddb_t *itunesdb, int tihm_num, dohm_t *dohm);
/* eq is an integer specifier from TunesEQPresets */
int  db_song_modify_eq(ipoddb_t *itunesdb, u_int32_t tihm_num, int eq);
int  db_song_list (ipoddb_t *itunesdb, db_list_t **head);
void db_song_list_free (db_list_t **head);
int  db_song_hide (ipoddb_t *itunesdb, u_int32_t tihm_num);
int  db_song_unhide (ipoddb_t *itunesdb, u_int32_t tihm_num);

/* itunesdb2/image_list.c */
int  db_photo_add (ipoddb_t *artworkdb, u_int8_t *image_data, size_t image_size, u_int64_t id);
int  db_photo_list (ipoddb_t *artworkdb, db_list_t **head);
void db_photo_list_free (db_list_t **head);

int    db_set_debug (ipoddb_t *itunesdb, int level, FILE *out);

/* make sure all the values contained in the tihm are correct, there is currently no
   checks so you could screw up a working song entry */
int    db_song_modify (ipoddb_t *itunesdb, int tihm_num, tihm_t *tihm);

/* returns the tihm number of the first match to data of dohm_type */
int    db_lookup (ipoddb_t *itunesdb, int dohm_type, char *data);
/* returns the playlist number of first match */
int    db_lookup_playlist (ipoddb_t *itunesdb, char *data);

int db_playlist_number      (ipoddb_t *itunesdb);
int db_playlist_create      (ipoddb_t *itunesdb, char *name);
int db_playlist_rename      (ipoddb_t *itunesdb, int playlist, u_int8_t *name);
int db_playlist_delete      (ipoddb_t *itunesdb, int playlist);
int db_playlist_tihm_add    (ipoddb_t *itunesdb, int playlist, int tihm_num);
int db_playlist_tihm_remove (ipoddb_t *itunesdb, int playlist, int tihm_num);
int db_playlist_clear       (ipoddb_t *itunesdb, int playlist);
int db_playlist_fill        (ipoddb_t *itunesdb, int playlist);
int db_playlist_remove_all  (ipoddb_t *itunesdb, int tihm_num);
int db_playlist_get_name    (ipoddb_t *itunesdb, int playlist,
			     char **name);

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
int db_playlist_list (ipoddb_t *itunesdb, db_list_t **head);
int db_playlist_song_list (ipoddb_t *itunesdb, int playlist, db_list_t **head);

int db_album_number       (ipoddb_t *photodb);
int db_album_list         (ipoddb_t *photodb, db_list_t **head);
int db_album_image_remove (ipoddb_t *photodb, int album, int image_id);
int db_album_image_add    (ipoddb_t *photodb, int album, int image_id);
int db_album_create       (ipoddb_t *photodb, u_int8_t *name);


int dohm_add (tihm_t *timh, u_int8_t *data, int data_len, char *encoding, int data_type);

/* functions for cleaning up memory */
void tihm_free (tihm_t *tihm);
void iihm_free (iihm_t *iihm);
void inhm_free (inhm_t *inhm);
void db_free   (ipoddb_t *itunesdb);

void db_playlist_list_free (db_list_t **head);
void db_album_list_free (db_list_t **head);
void db_playlist_song_list_free (db_list_t **head);

#if defined(__cplusplus)
}
#endif

#endif
