/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 create.c
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "itunesdbi.h"

#define DBHM_HEADER_SIZE 0x68
#define DFHM_HEADER_SIZE 0x84
#define ILHM_HEADER_SIZE 0x5c
#define ALHM_HEADER_SIZE 0x5c
#define FLHM_HEADER_SIZE 0x5c
#define TLHM_HEADER_SIZE 0x5c
#define PLHM_HEADER_SIZE 0x5c

static int db_dbhm_create (tree_node_t **entry, int flags) {
  struct db_dbhm *dbhm_data;
  int ret;

  if ((ret = db_node_allocate (entry, DBHM, DBHM_HEADER_SIZE, DBHM_HEADER_SIZE)) < 0)
    return ret;
  dbhm_data = (struct db_dbhm *) (*entry)->data;

  /* these are the values seen in iTunes iTunesDBs */
  dbhm_data->unk0           = 0x1;
  dbhm_data->unk3           = rand ();
  dbhm_data->unk4           = rand ();
  dbhm_data->unk5           = 0x00000001;
  dbhm_data->itunes_version = 0x00000010;
  dbhm_data->num_dshm       = 0x2;

  return 0;
}

static int db_dfhm_create (tree_node_t **entry) {
  struct db_dfhm *dfhm_data;
  int ret;

  if ((ret = db_node_allocate (entry, DFHM, DFHM_HEADER_SIZE, DFHM_HEADER_SIZE)) < 0)
    return ret;

  dfhm_data = (struct db_dfhm *) (*entry)->data;

  dfhm_data->unk1      = 0x00000001;
  dfhm_data->num_dshm  = 0x00000003;

  /* This is the value used by iTunes */
  dfhm_data->next_iihm = 0x00000064;

  return 0;
}

static int db_ilhm_create (tree_node_t **entry) {
  return db_node_allocate (entry, ILHM, ILHM_HEADER_SIZE, 0);
}

static int db_alhm_create (tree_node_t **entry) {
  return db_node_allocate (entry, ALHM, ALHM_HEADER_SIZE, 0);
}

static int db_flhm_create (tree_node_t **entry) {
  return db_node_allocate (entry, FLHM, FLHM_HEADER_SIZE, 0);
}

static int db_tlhm_create (tree_node_t **entry) {
  return db_node_allocate (entry, TLHM, TLHM_HEADER_SIZE, 0);
}

static int db_plhm_create (tree_node_t **entry) {
  return db_node_allocate (entry, PLHM, PLHM_HEADER_SIZE, 0);
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
int db_create (ipoddb_t *itunesdb, char *db_name, int name_len, int flags) {
  tree_node_t *root, *entry, *entry2;
  int ret;

  if ((itunesdb == NULL) || (db_name == NULL) || (name_len < 1))
    return -EINVAL;

  db_log (itunesdb, 0, "db_create: entering...\n");

  db_dbhm_create (&root, flags);

  /* create song list */
  db_dshm_create (&entry, 1); /* type 1 is the track list */
  db_attach (root, entry);

  db_tlhm_create (&entry2);
  db_attach (entry, entry2);

  /* create master playlist */
  db_dshm_create (&entry, 2); /* type 2 is the playlist list */
  db_attach (root, entry);

  db_plhm_create (&entry2);
  db_attach (entry, entry2);

  itunesdb->tree_root = root;
  itunesdb->flags = flags;
  itunesdb->type  = 0;

  ret = db_playlist_create (itunesdb, db_name, name_len);

  db_log (itunesdb, 0, "db_create: complete\n");

  return ret;
}

int db_photo_create (ipoddb_t *photodb) {
  tree_node_t *root, *entry, *entry2;

  if (photodb == NULL)
    return -EINVAL;

  db_log (photodb, 0, "db_photo_create: entering (photodb = %08x)...\n", photodb);

  db_dfhm_create (&root);

  /* create image list */
  db_dshm_create (&entry, 1); /* type 1 is the image list */
  db_attach (root, entry);

  db_ilhm_create (&entry2);
  db_attach (entry, entry2);

  /* create album list */
  db_dshm_create (&entry, 2); /* type 2 is the album list */
  db_attach (root, entry);

  db_alhm_create (&entry2);
  db_attach (entry, entry2);

  /* create photo list */
  db_dshm_create (&entry, 3); /* type 3 is the fileid list */
  db_attach (root, entry);

  db_flhm_create (&entry2);
  db_attach (entry, entry2);

  photodb->tree_root = root;
  photodb->type = 1;

  db_log (photodb, 0, "db_photo_create: complete\n");

  return 0;
}

