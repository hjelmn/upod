/**
 *   (c) 2002-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.4.0 create.c
 *
 *   Contains db_create and various node creation routines (that dont have another home).
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

static int db_dbhm_create (tree_node_t **entry, int flags) {
  struct db_dbhm *dbhm_data;
  int ret;

  if ((ret = db_node_allocate (entry, DBHM, DBHM_CELL_SIZE, DBHM_CELL_SIZE)) < 0)
    return ret;
  dbhm_data = (struct db_dbhm *) (*entry)->data;

  /* these are the values seen in iTunes iTunesDBs */
  dbhm_data->unk0           = 0x1;
  dbhm_data->unk3           = rand ();
  dbhm_data->unk4           = rand ();
  dbhm_data->unk5           = 0x00000001;
  dbhm_data->itunes_version = 0x00000010;

  return 0;
}

static int db_dfhm_create (tree_node_t **entry) {
  struct db_dfhm *dfhm_data;
  int ret;

  if ((ret = db_node_allocate (entry, DFHM, DFHM_CELL_SIZE, DFHM_CELL_SIZE)) < 0)
    return ret;

  dfhm_data = (struct db_dfhm *) (*entry)->data;

  dfhm_data->unk1      = 0x00000001;

  /* This is the value used by iTunes */
  dfhm_data->next_iihm = 0x00000064;

  return 0;
}

/**
  db_create:

  Creates an empty itunesdb.

  Arguments:
   ipoddb_t *itunesdb - pointer to where the database should be loaded.
   char       *db_name  - name of this database, usually the name of your ipod
   int         name_len - length of db_name

  Returns:
   < 0 on error
     0 on success
**/
int db_create (ipoddb_t *itunesdb, u_int8_t *db_name, u_int8_t *path, int flags) {
  int ret;

  if ((itunesdb == NULL) || (db_name == NULL))
    return -EINVAL;

  db_log (itunesdb, 0, "db_create: entering...\n");

  db_dbhm_create (&itunesdb->tree_root, flags);

  /* create song list (first data section) */
  db_dshm_add (itunesdb, TLHM, 1);

  /* create master podcast playlist (third data section) which NEEDS to be 2nd
     for some bizare reason */
  db_dshm_add (itunesdb, PLHM, 3);
  db_playlist_create (itunesdb, db_name, 3);
  db_playlist_create_podcast (itunesdb, "Podcasts", 3);

  /* create master music playlist (second data section) */
  db_dshm_add (itunesdb, PLHM, 2);
  db_playlist_create (itunesdb, db_name, 2);

  /* a podcast playlist also exists in the 2nd data section */
  db_playlist_create_podcast (itunesdb, "Podcasts", 2);



  itunesdb->flags = flags;
  itunesdb->type  = 0;
  itunesdb->path  = strdup ((char *)path);

  db_log (itunesdb, 0, "db_create: complete\n");

  return 0;
}

int db_photo_create (ipoddb_t *photodb, u_int8_t *path) {
  if (photodb == NULL || path == NULL)
    return -EINVAL;

  db_log (photodb, 0, "db_photo_create: entering (photodb = %08x)...\n", photodb);

  db_dfhm_create (&photodb->tree_root);

  /* create image list (first data section*/
  db_dshm_add (photodb, ILHM, 1);

  /* create album list (second data section) */
  db_dshm_add (photodb, ALHM, 2);

  /* create photo list (third data section) */
  db_dshm_add (photodb, FLHM, 3);

  photodb->type = 1;
  photodb->path = strdup ((char *)path);

  db_album_create (photodb, (u_int8_t *)"Artwork");

  db_log (photodb, 0, "db_photo_create: complete\n");

  return 0;
}

