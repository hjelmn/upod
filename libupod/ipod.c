/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.3 ipod.c
 *
 *   Routines for working with the ipod the iPod's.
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

#include "itunesdbi.h"

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <fcntl.h>

#define ITUNESDB_PATH "iPod_control/iTunes/iTunesDB"

static int ipod_init_files (ipod_t *ipod, char *dir) {
  int test_fd;
  /* 0755 */
  int fold_perms = S_IXUSR | S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IXOTH | S_IROTH;
  char testpath[512];

  /* try iPod_control/ */
  sprintf (testpath, "%s/iPod_control", dir);
  if ((test_fd = open (testpath, O_RDONLY)) < 0) {
    /* create iPod folder */
    UPOD_DEBUG(0, "Creating dir %s\n", testpath);
    if (mkdir (testpath, fold_perms) < 0)
      perror("mkdir");
  } else
    close (test_fd);

  /* try iPod_control/iTunes/ */
  sprintf (testpath, "%s/iPod_control/iTunes", dir);
  if ((test_fd = open (testpath, O_RDONLY)) < 0) {
    /* create iPod/iTunes */
    UPOD_DEBUG(0, "Creating dir %s\n", testpath);
    if (mkdir (testpath, fold_perms) < 0)
      perror("mkdir");
  } else
    close (test_fd);

  /* try iPod_control/iTunes/iTunesDB */
  sprintf (testpath, "%s/%s", dir, ITUNESDB_PATH);
  if (db_load (&(ipod->itunesdb), testpath) < 0) {
    UPOD_DEBUG(0, "iTunesDB not found, creating one\n");
    db_create (&(ipod->itunesdb), "iPod", 4);
  }

  /* try iPod/iPodPrefs */

  return 0;
}

static int ipod_mount (char *dev, char *fstype, char *dir) {
  /* -- XXX -- check if the ipod is already mounted */
  /*
  if (mount (dev, dir, fstype, 0xC0ED0000, NULL) < 0) {
    perror ("ipod_mount");
    return -1;
  }
  */
  /* -- XXX -- There may be other mount-time stuff that needs
     to be done here */

  return 0;
}

int ipod_open (ipod_t *ipod, char *dir, char *dev, char *fstype) {
  if (ipod == NULL || dir == NULL)
    return -1;

  /*
  if (dev != NULL && fstype != NULL)
    if (ipod_mount(dev, fstype, dir) < 0)
      return -1;
  */
  /* copy path argument into ipod structure */
  if (ipod->dir)
    free (ipod->dir);

  ipod->dir = calloc (strlen (dir) + 1, 1);
  memcpy (ipod->dir, dir, strlen (dir));

  /* copy itunedb location into ipod structure */
  if (ipod->itunesdb_path)
    free (ipod->itunesdb_path);

  ipod->itunesdb_path = calloc (strlen (dir) + strlen (ITUNESDB_PATH) + 2, 1);
  sprintf (ipod->itunesdb_path, "%s/%s", dir, ITUNESDB_PATH);
  printf ("DB: %s\n", ipod->itunesdb_path);
  /* memset (itunedb, 0); */

  /* open the itunesdb or possibly create it */
  ipod_init_files (ipod, dir);

  return 0;
}

int ipod_close (ipod_t *ipod) {
  /* if (mounted by upod) */
  //umount (ipod->dir);

  if (db_write (ipod->itunesdb, ipod->itunesdb_path) < 0)
    printf ("DB: Not written\n");
  else
    printf ("DB: Written to file %s\n", ipod->itunesdb_path);


  /* -- XXX -- free all allocated memory */
  free (ipod->dir);
  free (ipod->itunesdb_path);
  
  return -1;
}

int ipod_copy_from (ipod_t *ipod, char *topath, char *frompath) {
  return -1;
}

int ipod_copy_to (ipod_t *ipod, char *topath, char *frompath) {
  return -1;
}

int ipod_rename (ipod_t *ipod, char *name, int name_len) {
  return db_playlist_rename (&(ipod->itunesdb), 0, name, name_len);
}

/*** NTH_87Xz10102 ***/
