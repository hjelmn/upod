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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "upodi.h"

#include <stdio.h>
#include <stdlib.h>

void bswap_block (char *ptr, size_t membsize, size_t nmemb) {
  int i;

#if BYTE_ORDER == BIG_ENDIAN
  for (i = 0 ; i < nmemb ; i++)
    switch (membsize) {
    case 2:
      {
	short *r = (short *)ptr;
	/* may be needed for unicode strings */
	r[i] = bswap_16 (r[i]);
      }
    case 4:
      {
	long *r = (long *)ptr;
	r[i] = bswap_32 (r[i]);
      }
    }
#endif
}
