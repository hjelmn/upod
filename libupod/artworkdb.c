/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1alpha artworkdb.c
 *
 *   Functions to manipulate the list of artwork in an ArtworkDB.
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

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/uio.h>
#include <unistd.h>

#include <fcntl.h>

#include <errno.h>

#include "itunesdbi.h"

int db_photo_list (ipoddb_t *artworkdb, GList **head) {
  return -1; /* Not yet implemented */
}

void db_photo_list_free (GList **head) {
}
