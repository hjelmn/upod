/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a ipod.c
 *
 *   Routines for copying the file to the ipod (if not already on it and
 *  calling the functions to manipulate the iTunesDB (contained in db.c).
 *
 *  (6-14-2002):
 *    - With a sudden burst of inpiration (and alot of caffene) ipod.c is
 *     started. Initial implementations include:
 *      * ipod_open
 *      * ipod_close
 *      * ipod_add
 *      * ipod_remove
 *      * ipod_move
 *      * ipod_copy
 *    All the above funtions require that the ipod is mounted read write.
 *   future versions will be based off of hfsplus utils until the hfs+
 *   kernel module's completion.
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

#include "upodi.h"

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <errno.h>

static int ipod_debug = 1;

#define PATH_MAX         255

/* some default defines for working with the directory structure of the ipod */
#define DB_PATH          "/iPod_control/iTunes/iTunesDB"
#define DEFAULT_ADD_PATH "/Music"

/* dont normally want to add here as it is hidden by default */
#define APPLE_ADD_PATH   "/iPod_control/Music" /* followed by F[01][0-9] */

/*
  ipod_open: Opens an ipod's db and sets up the ipod_t structure

  Under darwin the path should be somthing like:
  /Volumes/<name of ipod>
*/
int ipod_open (ipod_t *ipod, char *path) {
  char db_path[PATH_MAX];

  if (ipod == NULL) return -1; /* cant do anything if there is no place to put it */

  memset (ipod, 0, sizeof(ipod_t));

  /* i want these to be snprintf, but macosx says it is undefined */
  snprintf (db_path, PATH_MAX, "%s/%s\0", path, DB_PATH);
  snprintf (ipod->path, PATH_MAX, "%s/%s\0", path, DB_PATH);
  snprintf (ipod->prefix, PATH_MAX, "%s", path);

  if (db_load (ipod, db_path) < 0) {
    UPOD_ERROR (-1, "ipod_open\n");
    return -1;
  }

  return 0;
}

int ipod_close (ipod_t *ipod) {
  if (db_write(*ipod, ipod->path) < 0) {
    UPOD_ERROR (-1, "ipod_close\n");
    return -1;
  }

  return 0;
}

int my_hash (char *ptr) {
  int len = strlen(ptr);
  int ret = 0;
  int i;

  for (i = 0 ; i < len ; i++)
    ret = ret<< 8 + ptr[i];

  return ret%20;
}

int ipod_add (ipod_t *ipod, char *filename, char *mac_path, int use_apple_path) {
  char final_mac_path[PATH_MAX];
  char dst_path[PATH_MAX];
  char *argv[4];
  char command[] = "/bin/cp";
  char force[] = "-r";
  
  if (ipod == NULL) return -1;
  if (filename == NULL) return -2;

  if (use_apple_path)
    snprintf (final_mac_path, PATH_MAX, ":%s:F%02i:%s\0", APPLE_ADD_PATH, my_hash(basename(filename)), basename(filename));
  else if (mac_path == NULL)
    snprintf (final_mac_path, PATH_MAX, ":%s:%s\0", DEFAULT_ADD_PATH, basename(filename));
  else
    snprintf (final_mac_path, PATH_MAX, "%s\0", mac_path);

  snprintf (dst_path, PATH_MAX, "%s/\0", ipod->prefix);
  path_mac_to_unix (final_mac_path, &dst_path[strlen(dst_path) - 1]);

  argv[0] = command;
  argv[1] = force;
  argv[2] = filename;
  argv[3] = dst_path;

  execv (command, argv);

  if (db_add (ipod, filename, final_mac_path) < 0) {
    printf("File copied to ipod, but not added to iTunesDB.\n");

    return 1;
  }

  return 0;
}

int ipod_remove (ipod_t *ipod, char *mac_path, int remove_from_db, int remove_file) {
  char path[PATH_MAX];
  char *argv[3];
  char command[] = "/bin/rm";
  char opt[] = "-f";

  if (remove_file) {
    snprintf (path, PATH_MAX, "%s/\0", ipod->prefix);
    path_mac_to_unix (mac_path, &path[strlen(path) - 1]);
    argv[0] = command;
    argv[1] = opt;
    argv[2] = path;

    execv (command, argv);
  }

  if (remove_from_db) {
    if (db_remove(ipod, db_lookup(*ipod, IPOD_PATH, mac_path, strlen(mac_path))) < 0)
      return -1;
  }

  return 0;
}

int ipod_move (ipod_t *ipod, char *src, char *dst) {
  char *argv[3];
  char command[] = "/bin/mv";
  char src_path[PATH_MAX];
  char dst_path[PATH_MAX];

  if (strchr(dst, ':')) {
    snprintf (dst_path, PATH_MAX, "%s/\0", ipod->prefix);
    path_mac_to_unix (dst, &dst_path[strlen(dst_path) - 1]);
  } else {
    snprintf (dst_path, PATH_MAX, "%s\0", dst);
  }

  if (strchr(src, ':')) {
    snprintf (src_path, PATH_MAX, "%s/\0", ipod->prefix);
    path_mac_to_unix (src, &src_path[strlen(src_path) - 1]);
  } else {
    snprintf (src_path, PATH_MAX, "%s\0", src);
  }

  argv[0] = command;
  argv[1] = src_path;
  argv[2] = dst_path;

  execv (command, argv);

  return 0;
}

int ipod_copy (ipod_t *ipod, char *src, char *dst) {
  char *argv[3];
  char command[] = "/bin/cp";
  char src_path[PATH_MAX];
  char dst_path[PATH_MAX];

  if (strchr(dst, ':')) {
    snprintf (dst_path, PATH_MAX, "%s/\0", ipod->prefix);
    path_mac_to_unix (dst, &dst_path[strlen(dst_path) - 1]);
  } else {
    snprintf (dst_path, PATH_MAX, "%s\0", dst);
  }

  if (strchr(src, ':')) {
    snprintf (src_path, PATH_MAX, "%s/\0", ipod->prefix);
    path_mac_to_unix (src, &src_path[strlen(src_path) - 1]);
  } else {
    snprintf (src_path, PATH_MAX, "%s\0", src);
  }

  argv[0] = command;
  argv[1] = src_path;
  argv[2] = dst_path;

  execv (command, argv);

  return 0;
}
