/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.2 endian.c
 *
 *   upod endianness functions
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

#if defined (linux)
#include <endian.h>
#include <byteswap.h>
#endif

#include "upod.h"

u_int32_t bswap32 (u_int32_t x) {
  return ( ( (x & 0xff000000) >> 24) ||
	   ( (x & 0x00ff0000) >>  8) ||
	   ( (x & 0x0000ff00) <<  8) ||
	   ( (x & 0x000000ff) << 24) );
}

void swap32(u_int32_t *ptr) {
  *ptr = bswap_32(*ptr);
}
