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

#include "itunesdbi.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

void char_to_unicode (char *dst, char *src, size_t src_length) {
  int i;
  u_int16_t *dst_uni = (u_int16_t *)dst;
  
  memset (dst, 0, src_length * 2);

  for (i = 0 ; i < src_length ; i++)
    dst_uni[i] = src[i];
}

void unicode_to_char (char *dst, char *src, size_t src_length) {
  int i;
  u_int16_t *src_uni = (u_int16_t *)src;
  
  memset(dst, 0, src_length/2 + 1);

  for (i = 0 ; i < src_length/2 ; i++)
    dst[i] = (char)src_uni[i];
}

void unicode_check_and_copy (char **dst, int *dst_len, char *src,
			     int src_len) {
  if (dst == NULL || dst_len == NULL || src_len == 0) return;
  
#if BYTE_ORDER == BIG_ENDIAN
  if (src[0] != '\0') {
#else
  if (src[1] != '\0') {
#endif
    *dst_len = 2 * src_len;
    *dst     = (char *) calloc (src_len, sizeof(u_int16_t));
    char_to_unicode (*dst, src, src_len);
  } else {
    *dst_len = src_len;
    *dst     = (char *) calloc (src_len/2, sizeof(u_int16_t));
    
    memcpy (*dst, src, src_len);
  }
}
  
int unicodencasecmp (char *string1, int string1_len, char *string2, int string2_len) {
  int i;

  if (string2_len > string1_len)
    return 1;

  for (i = 0 ; i < string2_len ; i++) {
    if (strncasecmp (&string1[i], &string2[i], 1) != 0) {
      if (string1_len > string2_len)
	return unicodencasecmp (&string1[1], string1_len - 1, string2, string2_len);
      else
	return -1;
    }

  }

  return 0;
}
