/**
 *   (c) 2002-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
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

/* this source file is not currently being used */

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

#include "itunesdb.h"

int device_info_write (ipod_t *ipod);
int sysinfo_read (ipod_t *, char *);

static int upod_checkdir (char *dir, int debug_level, FILE *debug_out) {
  int test_fd;
  int dir_perms = S_IXUSR | S_IRUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IXOTH | S_IROTH;

  if ((test_fd = open (dir, O_RDONLY)) < 0) {
    if (debug_level > 0)
      fprintf (debug_out, "libupod/ipod.c upod_checkdir: creating directory %s\n", dir);

    if (mkdir (dir, dir_perms) < 0) {
      perror("mkdir");

      return -errno;
    }

  } else
    close (test_fd);

  return 0;
}

static int upod_init (ipod_t *ipod, int debug_level, FILE *debug_out) {
  int ret, sw_interface_vers;
  /* 0755 */
  char testpath[512];
  char *dir = ipod->path;
  artwork_flag_t aw_flag;

  /* Check that iPod_Control exists on the iPod */
  sprintf (testpath, "%s/iPod_Control", dir);
  if (ret < 0)
    return ret;

  /* Try to read the iPod's system info */
  sprintf (testpath, "%s/iPod_Control/Device/SysInfo", dir);
  if (sysinfo_read (ipod, testpath) == 0) {
    sw_interface_vers = strtol (ipod->sw_interface, NULL, 16);

    /* Determine iPod generation from the software interface version */
    switch (sw_interface_vers >> 16) {
    case 0x000C:
      if (debug_level > 0)
	fprintf (debug_out, "libupod/ipod.c upod_init: iPod Nano detected\n");

      aw_flag = UPOD_NANOART;
      break;
    case 0x000b:
      if (debug_level > 0)
	fprintf (debug_out, "libupod/ipod.c upod_init: iPod Video detected\n");

      aw_flag = UPOD_VIDEOART;
      break;
    case 0x0006:
      if (debug_level > 0)
	fprintf (debug_out, "libupod/ipod.c upod_init: iPod Photo detected\n");

      aw_flag = UPOD_PHOTOART;
      break;
    default:
      /* Unsupported or pre-photo iPod */
      if (debug_level > 0)
	fprintf (debug_out, "libupod/ipod.c upod_init: iPod model may not support artwork\n");

      ipod->supports_artwork = 0;
    }
  } else
    /* iPod shuffle or missing SysInfo file */
    ipod->supports_artwork = 0;

  /* Check for iTunes data dir */
  sprintf (testpath, "%s/iPod_Control/iTunes", dir);
  ret = upod_checkdir (testpath, debug_level, debug_out);
  if (ret < 0)
    return ret;

  /* Prepare ipod db structure and set uPod debug level */
  memset (&(ipod->itunesdb), 0, sizeof (ipoddb_t));
  db_set_debug (&(ipod->itunesdb), debug_level, debug_out);

  /* Open/Create the iTunes database */
  sprintf (testpath, "%s/%s", dir, ITUNESDB);
  if (db_load (&(ipod->itunesdb), testpath, 0x1) < 0) {
    if (debug_level > 0)
      fprintf(debug_out, "iTunesDB not found, creating one\n");

    db_create (&(ipod->itunesdb), (u_int8_t *)"iPod", (u_int8_t *)testpath, 0);
  }

  if (ipod->supports_artwork == 1) {
    memset (&(ipod->artworkdb), 0, sizeof (ipoddb_t));
    memset (&(ipod->photodb), 0, sizeof (ipoddb_t));

    db_set_debug (&(ipod->artworkdb), debug_level, debug_out);
    db_set_debug (&(ipod->photodb), debug_level, debug_out);

    /* Check for Artwork directory. */
    sprintf (testpath, "%s/iPod_Control/Artwork", dir);
    ret = upod_checkdir (testpath, debug_level, debug_out);
    if (ret < 0)
      return ret;

    /* Open/Create the artwork database. */
    sprintf (testpath, "%s/%s", dir, ARTWORKDB);
    if (db_load (&(ipod->artworkdb), testpath, 0) < 0) {
      fprintf(debug_out, "ArtworkDB not found, creating one\n");
      db_photo_create (&(ipod->artworkdb), (u_int8_t *)testpath);
    }

    ipod->artworkdb.supports_artwork = aw_flag;

    /* Check for photo directory. */
    sprintf (testpath, "%s/Photos", dir);
    ret = upod_checkdir (testpath, debug_level, debug_out);
    if (ret < 0)
      return ret;

    /* Open/Create the photo database. uPod does not support photos yet. */
    /*
      sprintf (testpath, "%s/%s", dir, PHOTODB);
      if (db_load (&(ipod->photodb), testpath, 0) < 0) {
      fprintf(debug_out, "Photo Database not found, creating one\n");
      db_photo_create (&(ipod->photodb), testpath);
      
      db_album_create (&ipod->photodb, "iPod photos");
    }
    */
  }
  /* try iPod/iPodPrefs */

  return 0;
}

int upod_open (ipod_t *ipod, char *dir, int debug_level, FILE *debug_out) {
  int ret;

  if (ipod == NULL || dir == NULL)
    return -EINVAL;

  /* open the itunesdb or possibly create it */
  ret = upod_init (ipod, debug_level, debug_out);
  if (ret < 0)
    return ret;

  /* copy path argument into ipod structure */
  ipod->path = strdup (dir);

  return 0;
}

int upod_close (ipod_t *ipod) {
  int ret = 0;

  ret = device_info_write (ipod);

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

#if 0
int ipod_copy_from (ipod_t *ipod, char *topath, char *frompath) {
  return -1;
}

int ipod_copy_to (ipod_t *ipod, char *topath, char *frompath) {
  return -1;
}

int ipod_rename (ipod_t *ipod, u_int8_t *name) {
  return db_playlist_rename (&(ipod->itunesdb), 0, name);
}

#endif
