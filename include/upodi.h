/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.2 upodi.h
 *
 *   Internal functions. Do not include upodi.h in any end software.
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
#ifndef __UPODI_H
#define __UPODI_H

#define DEBUG

#if defined(DEBUG)
#define UPOD_DEBUG printf
#else
#define UPOD_DEBUG
#endif

/* this definition will not be static in non-alpha release */
#define EMPTYDB "/home/neo/cvs/upod/share/emptydb"

#define UPOD_ERROR(x, s) do {fprintf(stderr, "Error %i: %s", x, s);} while(0);

enum file_types_t {TITLE=1, PATH, ALBUM, ARTIST, GENRE, TYPE, COMMENT};

typedef struct _ipod {
  /* mount information */
  char path[255];
  char is_mounted;

  int db_fd;

  int db_size_current;

  int debug;

  char *first_entry;
  char *last_entry;
  
  int num_playlists;
  
  /* this pointer could possibly be huge (the size of the database) */
  char *itunesdb;

  char *insertion_point;
} ipod_t;

typedef struct _song_ref {
  char *db_location;

  int size;
  int time;

  int bitrate;
  int samplerate;

  int mod_date;
  
  int dohm_num;
  struct dohm {
    int type;
    char *data;
  } *dohm_entries;

  struct _song_ref *prev, *next;
} song_ref_t;

/* field seperator should be : */
char *path_mac_to_unix (char *path);
/* field seperator should be / */
char *path_unix_to_mac (char *path);

char *db_get_path(ipod_t *ipod, song_ref_t *ref);

/* libupod/mp3.c */
int ref_fill_mp3 (char *, song_ref_t *);
void ref_clear (song_ref_t *);
#endif /* __UPODI_H */
