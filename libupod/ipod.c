/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.2 ipod.c
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

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>

#include <sys/stat.h>
#include <sys/mount.h>

#include <stdarg.h>
#include <libgen.h>
#include <string.h>

#include <errno.h>

#include <byteswap.h>

#include "upod.h"
#include "upodi.h"

int upod_debug(char *fmt, ...) {
}

int open_itunesdb (char *device, char *mnt_path) {
  char tmp[strlen(mnt_path) + 29];

  umount(device);
  mount(device, mnt_path, "hfsplus", 0, NULL);

  memset(tmp, 0, strlen(mnt_path) + 29);

  sprintf(tmp, "%s/iPod_control/iTunes/iTunesDB", mnt_path);

  return open(tmp, O_RDWR);
}

int close_itunesdb (int fd) {
  return close(fd);
}

int ipod_copy (ipod_t *ipod, char *src, char *dst, int add_to_db) {
  char copy_command[] = "/bin/cp";
  char option[] = "-f";
  char *argv[4];

  int ret;

  /* cp options */
  argv[0] = copy_command;
  argv[1] = option;
  argv[2] = src;
  argv[3] = dst;

  if (execv(copy_command, argv) != 0) {
    perror("ipod_copy:");
    return errno;
  }

  if (add_to_db) {
    return db_add(ipod, dst);
  }

  return 0;
}

int ipod_copy_to (ipod_t *ipod, char *src) {
  char *src_name = basename(src);
  char dst[strlen(ipod->path) + strlen(src_name) + 2];

  sprintf(dst, "%s/%s", ipod->path, src_name);

  return ipod_copy(ipod, src, dst, 1);
}

int ipod_copy_from (ipod_t *ipod, song_ref_t *ref, char *dst) {
  return -1;
}

int ipod_delete (ipod_t *ipod, song_ref_t *ref) {
  char *argv[3];
  char delete_cmd[] = "/bin/rm";
  char delete_opt[] = "-f";
  char *path;

  argv[0] = delete_cmd;
  argv[1] = delete_opt;
  // get song path
  path = db_get_path(ipod, ref);
  argv[2] = path;

  // run rm
  if (execv(delete_cmd, argv) != 0) {
    perror ("ipod_delete");
    return errno;
  }

  // remove entry from iTunesDB
  return db_remove(ipod, ref);
}
