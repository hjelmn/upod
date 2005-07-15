/**
 *   (c) 2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1 device_info.c
 *
 *   Functions for managing playlists on the iPod.
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

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/stat.h>

#include "itunesdbi.h"

#define DEVICEINFO "/iPod_Control/iTunes/DeviceInfo"

/*
  DeviceInfo is a little-endian file containing:
    u_int16_t   string_length (in characters)
    u_int16_t[] ipod_name (UTF-16LE)
    u_int16_t[] zeros (padding the file to 0x600 bytes)
*/

int device_info_write (ipod_t *ipod) {
  int fd, i;

  char file_name[255];
  u_int8_t *ipod_name;
  u_int16_t *unicode_name;
  size_t unicode_len;
  int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  int ret;

  unsigned short num_chars;

  if (ipod == NULL)
    return -EINVAL;

  /* Get the iPod's name (Name of 0th playlist) */
  if ((ret = db_playlist_get_name (&(ipod->itunesdb), 0, &ipod_name)) < 0)
    return ret;

  to_unicode (&unicode_name, &unicode_len, ipod_name, strlen (ipod_name), "UTF-8", UTF_ENC);

  free (ipod_name);

  sprintf (file_name, "%s%s", ipod->path, DEVICEINFO);
  
  if ((fd = open (file_name, O_WRONLY | O_CREAT | O_TRUNC, perms)) < 0) {
    perror ("device_info_write|open");
    free (unicode_name);

    return -errno;
  }

  num_chars = unicode_len / 2;
  num_chars = little16_2_arch16(num_chars);

  /* Change the endianness of the iPod's name if needbe. */
  bswap_block ((char *)unicode_name, 2, unicode_len / 2);

  fprintf (stderr, "num_chars = %04x\n", num_chars);

  /* String length */
  write (fd, &num_chars, 2);

  /* iPod Name */
  write (fd, unicode_name, unicode_len);

  free (unicode_name);

  num_chars = 0;

  /* Pad the file with 0's */
  for (i = unicode_len + 2 ; i < 0x600 ; i += 2)
    write (fd, &num_chars, 2);

  close (fd);

  return 0;
}

int device_info_read (ipod_t *ipod) {
  UPOD_NOT_IMPL("device_info_read");
}
