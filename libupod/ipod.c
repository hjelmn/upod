/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0alpha ipod.c
 *
 *   Top-level routines for working with iPods.
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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#include "upod.h"

int device_info_write (ipod_t *ipod);
int sysinfo_read (ipod_t *, char *);

static int ipod_init (ipod_t *ipod, int debug_level, FILE *debug_out) {
  int test_fd;
  /* 0755 */
  int fold_perms = S_IXUSR | S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IXOTH | S_IROTH;
  char testpath[512];
  char *dir = ipod->path;

  /* try iPod_Control/ */
  sprintf (testpath, "%s/iPod_Control", dir);
  if ((test_fd = open (testpath, O_RDONLY)) < 0) {
    /* create iPod folder */
    fprintf(stderr, "Creating dir %s\n", testpath);
    if (mkdir (testpath, fold_perms) < 0)
      perror("mkdir");
  } else
    close (test_fd);

  sprintf (testpath, "%s/iPod_Control/Device/SysInfo", dir);
  sysinfo_read (ipod, testpath);

  /* try iPod_Control/iTunes/ */
  sprintf (testpath, "%s/iPod_Control/iTunes", dir);
  if ((test_fd = open (testpath, O_RDONLY)) < 0) {
    /* create iPod/iTunes */
    fprintf(stderr, "Creating dir %s\n", testpath);
    if (mkdir (testpath, fold_perms) < 0)
      perror("mkdir");
  } else
    close (test_fd);

  /* try iPod_Control/iTunes/iTunesDB */
  memset (&(ipod->itunesdb), 0, sizeof (ipoddb_t));
  db_set_debug (&(ipod->itunesdb), debug_level, debug_out);
  sprintf (testpath, "%s/%s", dir, ITUNESDB);
  if (db_load (&(ipod->itunesdb), testpath, 0x1) < 0) {
    fprintf(stderr, "iTunesDB not found, creating one\n");
    db_create (&(ipod->itunesdb), "iPod", 0x1);
    ipod->itunesdb.path = strdup (testpath);
  }

  memset (&(ipod->artworkdb), 0, sizeof (ipoddb_t));
  db_set_debug (&(ipod->artworkdb), debug_level, debug_out);

  memset (&(ipod->photodb), 0, sizeof (ipoddb_t));
  db_set_debug (&(ipod->photodb), debug_level, debug_out);

  if (ipod->supports_artwork == 1) {
    sprintf (testpath, "%s/iPod_Control/Artwork", dir);
    if ((test_fd = open (testpath, O_RDONLY)) < 0) {
      fprintf(stderr, "Creating dir %s\n", testpath);
      if (mkdir (testpath, fold_perms) < 0)
	perror("mkdir");
    } else
      close (test_fd);

    sprintf (testpath, "%s/%s", dir, ARTWORKDB);
    if (db_load (&(ipod->artworkdb), testpath, 0) < 0) {
      fprintf(stderr, "ArtworkDB not found, creating one\n");
      db_photo_create (&(ipod->artworkdb));
      ipod->photodb.path = strdup (testpath);
    }

    sprintf (testpath, "%s/Photos", dir);
    if ((test_fd = open (testpath, O_RDONLY)) < 0) {
      fprintf(stderr, "Creating dir %s\n", testpath);
      if (mkdir (testpath, fold_perms) < 0)
	perror("mkdir");
    } else
      close (test_fd);

    sprintf (testpath, "%s/%s", dir, PHOTODB);
    if (db_load (&(ipod->photodb), testpath, 0) < 0) {
      fprintf(stderr, "Photo Database not found, creating one\n");
      db_photo_create (&(ipod->photodb));
      ipod->photodb.path = strdup (testpath);

      db_album_create (&ipod->photodb, "iPod photos");
    }
  }
  /* try iPod/iPodPrefs */

  return 0;
}

int ipod_open (ipod_t *ipod, char *dir, int debug_level, FILE *debug_out) {
  if (ipod == NULL || dir == NULL)
    return -EINVAL;

  /* copy path argument into ipod structure */
  ipod->path = strdup (dir);
  /* open the itunesdb or possibly create it */
  ipod_init (ipod, debug_level, debug_out);

  return 0;
}

int ipod_close (ipod_t *ipod) {
  int ret = 0;

  device_info_write (ipod);

  ret = db_write (ipod->itunesdb, ipod->itunesdb.path);

  db_free (&ipod->itunesdb);

  if (ipod->supports_artwork == 1) {
    ret = db_write (ipod->artworkdb, ipod->artworkdb.path);
    db_free (&ipod->artworkdb);

    ret = db_write (ipod->photodb, ipod->photodb.path);
    db_free (&ipod->photodb);
  }

  if (ipod->board)
    free (ipod->board);
  if (ipod->model_number)
    free (ipod->model_number);
  if (ipod->serial_number)
    free (ipod->serial_number);
  if (ipod->sw_version)
    free (ipod->sw_version);

  free (ipod->path);
  
  return 0;
}

int ipod_copy_from (ipod_t *ipod, char *topath, char *frompath) {
  return -1;
}

int ipod_copy_to (ipod_t *ipod, char *topath, char *frompath) {
  return -1;
}

int ipod_rename (ipod_t *ipod, u_int8_t *name) {
  return db_playlist_rename (&(ipod->itunesdb), 0, name);
}
