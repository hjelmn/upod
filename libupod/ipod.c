/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1 ipod.c
 *
 *   1/25/2002: initial revision
 *     - opens an iTunesDB when given a path to a iPod_control directory
 *     - dumps iTunesDB
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

#include <byteswap.h>

#include "upod.h"

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
