/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.0a unicode.c
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

void char_to_unicode (char *dst, char *src, size_t src_length) {
  int i;
  
  memset (dst, 0, src_length * 2);

  for (i = 0 ; i < src_length ; i++)
    dst[i * 2] = src[i];
}

void unicode_to_char (char *dst, char *src, size_t src_length) {
  int i;

  memset(dst, 0, src_length/2);

  for (i = 0 ; i < src_length/2 ; i++)
    dst[i] = src[i * 2];
}
