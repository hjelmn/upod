/**
 *   (c) 2004-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v1.0 sysinfo.c 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <libgen.h>

#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "itunesdbi.h"

int sysinfo_read (ipod_t *ipod, char *filename) {
  FILE *fh;
  char buffer[128];

  fh = fopen (filename, "r");

  /* Unknown iPods are assumed to support artwork */
  ipod->supports_artwork = 1;

  if (fh == NULL)
    return -errno;

  while (fgets (buffer, 128, fh)) {
    char *field, *value, *tmp;

    /* SysInfo contains lines with the format:
       field: value
    */
    field = strtok (buffer, ":");
    value = strtok (NULL,   ":");
    
    if (value == NULL)
      return -1;

    /* Skip whitespace in value */
    value++;

    tmp = strchr (value, '\n');
    if (tmp)
      *tmp = '\0';

    if      (strncmp (field, "BoardHwName", 11) == 0)
      ipod->board         = strdup (value);
    else if (strncmp (field, "ModelNumStr", 11) == 0)
      ipod->model_number  = strdup (value);
    else if (strncmp (field, "boardHwSwInterfaceRev", 21) == 0)
      ipod->sw_interface  = strdup (value);
    else if (strncmp (field, "pszSerialNumber", 15) == 0)
      ipod->serial_number = strdup (value);
    else if (strncmp (field, "visibleBuildID", 14) == 0) {
      value[strlen(value)-1] = '\0';
      ipod->sw_version = strdup (&value[12]);
    } else
      continue;
  }

  /* Non-color and mini iPods do not support artwork */
  if (strncmp (ipod->board, "iPod Q", 6) == 0)
    ipod->supports_artwork = 0;

  fclose (fh);

  return 0;
}

