/**
 *   (c) 2005-2006 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1 otg_playlist.c
 *
 *   Contains function for looking up a tihm entry in the iTunesDB
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

#include "itunesdbi.h"

int otg_playlist_read (ipod_t *ipod) {
  UPOD_NOT_IMPL("otg_playlist_read");
}

/*
  There is not reason to write an OTG playlist as it is maintained by
  the iPod itself. Instead, read the playlist and create an actual
  playlist from it, then delete the file.
*/
int otg_playlist_delete (ipod_t *ipod) {
  UPOD_NOT_IMPL("otg_playlist_delete");
}
