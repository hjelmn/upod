/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1 ipod.h
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


/* mhit */
typedef struct _iTunesDBentry {
  char      record_type[4];
  u_int32_t record_header_size;
  u_int32_t record_size;
  u_int32_t num_mhod;
  u_int32_t index;
  u_int32_t unk0;
  u_int32_t unk1;
  u_int32_t type;
  u_int32_t date;
  u_int32_t size;
  u_int32_t time;
  u_int32_t track;
} iTunesDBentry;

typedef struct _mhodentry {
  char      record_type[4];
  u_int32_t unk0;
  u_int32_t unk1;
  u_int32_t type;
  u_int32_t unk2;
  u_int32_t unk3;
  u_int32_t unk4;
  u_int32_t size;
  u_int32_t unk5;
  u_int32_t unk6;
} mhodentry;

typedef struct _iTunesPLSTentry {
  u_int32_t mhit;
  u_int32_t mhod;
} _iTunesPLSTentry;

struct entry_data {
  int size;
  int time;

  char *title;
  char *album;
  char *artist;

  /* this will be changed from using ':' for seperator to '/' */
  char *path;
};

struct song_list {
  struct song_list *prev;

  struct entry_data song;

  struct song_list *next;
};

typedef void *upod_instance;

upod_instance  *upod_init_instance(char *path_to_ipod, char *device);
int             db_add_song (char *);

