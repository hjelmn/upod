/**
 *   (c) 2003 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.1.1 unicode.c
 *
 *   convert to, from unicode, utf-8 using iconv
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
#if !defined(const)
#define const /* get rid of const (useless in c anyway?) */
#endif
#include <iconv.h>

#include "hexdump.c"

/* -- XXX -- Not used --- XXX --- */
void char_to_unicode (u_int16_t *dst, u_int8_t *src, size_t src_length) {
  int i;

  memset (dst, 0, src_length * 2);

  for (i = 0 ; i < src_length ; i++)
    dst[i] = src[i];
}

/* -- XXX -- this will be changed to be Unicode -> UTF8 -- XXX -- */
void unicode_to_char (u_int8_t *dst, u_int16_t *src, size_t src_length) {
  int i;
  
  memset(dst, 0, src_length/2 + 1);

  for (i = 0 ; i < src_length/2 ; i++)
    dst[i] = (u_int8_t)src[i];
}

void unicode_to_utf8 (u_int8_t **dst, size_t *dst_len, u_int16_t *src,
		      size_t src_len) {
  iconv_t conv;
  size_t final_size;
  char *inbuf, *outbuf;
  size_t inbytes, outbytes;
  
  if (dst == NULL || dst_len == NULL)
    return;

  if (src_len == 0) {
    *dst = NULL;
    return;
  }

  inbytes = src_len;
  inbuf   = (char *)src;

  /* UTF-8 encoding could be as large as 3 bytes for every 2 bytes
     of Unicode input */
  outbytes= (src_len/2)*3;
  *dst = outbuf  = calloc ((src_len/2)*3, 1);

  conv = iconv_open ("UTF-8", "UTF-16");
  iconv (conv, &inbuf, &inbytes, &outbuf, &outbytes);
  iconv_close (conv);
 
  final_size = (src_len/2)*3 - outbytes;

  *dst = realloc (*dst, final_size + 1);
  *dst_len = final_size;
}

void to_unicode_hack (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
		 size_t src_len, char *src_encoding) {
  int i, j;

  if (dst == NULL || dst_len == NULL)
    return;

  if (src_len == 0) {
    *dst = NULL;
    return;
  }
 
  *dst = calloc (1, src_len * 2);

  for (i = 0, j = 0 ; i < src_len ; i++) {
    if (!(src[i] && 0x8))
      j++;
    (*dst)[j++] = src[i];
  }

  *dst = realloc (*dst, j);
  *dst_len = j*2;
}

/* Converts the input from UTF8/ASCII to Unicode */
void to_unicode (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
		 size_t src_len, char *src_encoding) {
  iconv_t conv;
  size_t final_size;
  char *inbuf, *outbuf;
  size_t inbytes, outbytes;
  
  if (dst == NULL || dst_len == NULL)
    return;

  if (src_len == 0) {
    *dst = NULL;
    return;
  }

  inbytes = src_len;
  inbuf   = (char *)src;

  outbytes= src_len * 2;
  *dst = outbuf  = calloc (src_len * 2, 1);

  conv = iconv_open ("UTF-16BE", src_encoding);
  iconv (conv, &inbuf, &inbytes, &outbuf, &outbytes);
  iconv_close (conv);
 
  final_size = src_len * 2 - outbytes;

  *dst = realloc (*dst, final_size);
  *dst_len = final_size;
}

/*
void unicode_check_and_copy (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
			     size_t src_len) {
  int i, x;
  int length = 0;
  u_int8_t scratch[512];
  
  if (dst == NULL || dst_len == NULL || src_len == 0)
    return;

  *dst_len = 0;

   unicode id3 tags create by iTunes start with the 24 bit tag 0x01fffe (or, so I have noticed)
  if (src_len > 3 && (((int *)src)[0] & 0x01fffe00) == 0x01fffe00) {
    *dst_len = src_len-3;
    *dst = calloc (2, *dst_len/2);
    memmove (*dst, &src[3], *dst_len);

    bswap_block ((u_int8_t *)*dst, 2, *dst_len/2);
  } else {
    u_int16_t *dst16;

    dst16 = (u_int16_t *)scratch;
    
    for (i = 0, x = 0 ; i < src_len ; i++) {
      u_int8_t  onebyte    = src[i];
      
      if (src[i] == 0)
	continue;
      
      
      if (((src[i] & 0xe0) == 0xe0) &&
	  (i < (src_len - 1) && (src[i+1] & 0x80) == 0x80)) {
	dst16[x] = src[i+2] & 0x3f | ((src[i+1] & 0x3f) << 6) |
	  ((src[i] & 0x0f) << 12);
	i += 2;
      } else if (((src[i] & 0xc0) == 0xc0) &&
		 (i < (src_len - 1) && (src[i+1] & 0x80) == 0x80)) {
	dst16[x] = src[i+1] & 0x3f | ((src[i] & 0x1f) << 6);
	i += 1;
      } else
	dst16[x] = src[i];
      
      x++;
    }
    
    *dst_len = 2*x;
    *dst = calloc (2, x);
    if (*dst == NULL) {
      perror ("unicode_check_and_copy|calloc");
      *dst_len = 0;
      return;
    }

    memcpy (*dst, scratch, 2*x);
  }
}
*/  
int unicodencasecmp (u_int8_t *string1, size_t string1_len, u_int8_t *string2, size_t string2_len) {
  size_t i;

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
