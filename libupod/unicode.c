/** --*-c-mode-*--
 *   (c) 2003-2007 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3 unicode.c
 *
 *   convert to/from unicode/utf-8 using iconv
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

#if 0
#if !defined(const)
#define const /* get rid of const (useless in c anyway?) */
#endif

#include <iconv.h>

#warning "Using iconv"

/* Converts the input from UTF8/ASCII to Unicode */
void libupod_convstr (void **dst, size_t *dst_len, void *src,
		      size_t src_len, char *src_encoding, char *dst_encoding) {
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
  *dst = (u_int16_t *)outbuf  = calloc (src_len * 2, 1);

  conv = iconv_open (dst_encoding, src_encoding);
  iconv (conv, &inbuf, &inbytes, &outbuf, &outbytes);
  iconv_close (conv);
 
  final_size = src_len * 2 - outbytes;

  *dst = realloc (*dst, final_size + 1);
  *dst_len = final_size;
}
#else
static int encoding_utf8 (char *encoding) {
  return (strcmp (encoding, "UTF-8") == 0);
}

static int encoding_ascii (char *encoding) {
  return ((strcmp (encoding, "ASCII") == 0) ||
	  (strcmp (encoding, "ISO-8859-1") == 0));
}

static int encoding_utf16 (char *encoding) {
  return (strncmp (encoding, "UTF-16", 6) == 0);
}

static int encoding_equiv (char *encoding1, char *encoding2) {
  if (encoding_utf8 (encoding1) || encoding_ascii (encoding1)) {
    if (encoding_utf8 (encoding2) || encoding_ascii (encoding2))
      return 1;
  } else if (encoding_utf16 (encoding1) && encoding_utf16 (encoding2))
    return 1;

  return 0;
}

void libupod_convstr (void **dst, size_t *dst_len, void *src, size_t src_len,
		      char *src_encoding, char *dst_encoding) {
  u_int8_t scratch[1024];
  u_int16_t *dst16, *src16;
  u_int8_t *src8;
  int i, x, le, length;
  int dst_is_utf8, dst_is_ascii;

  dst_is_utf8  = encoding_equiv (dst_encoding, "UTF-8");
  dst_is_ascii = encoding_ascii (dst_encoding);

  if (encoding_equiv (src_encoding, dst_encoding)) {
    length = src_len;
  } else if (encoding_equiv (src_encoding, "UTF-8") && encoding_utf16 (dst_encoding)) {
    /* UTF-8/ASCII to UTF-16 */
    dst16 = (u_int16_t *)scratch;
    src8  = (u_int8_t *)src;

    for (i = 0, x = 0 ; x < 512 && i < src_len && src8[i] != '\0' ; x++) {
      if ((src8[i] & 0xe0) == 0xe0 && (i + 2 < src_len) && (src8[i+1] & 0xc0) == 0x80 && (src8[i+2] & 0xc0) == 0x80) {
	dst16[x] = (src8[i+2] & 0x3f) | ((src8[i+1] & 0x3f) << 6) | ((src8[i] & 0x0f) << 12);
	i += 3;
      } else if ((src8[i] & 0xc0) == 0xc0 && (i + 1 < src_len) && (src8[i+1] & 0xc0) == 0x80) {
	dst16[x] = (src8[i+1] & 0x3f) | ((src8[i] & 0x1f) << 6);
	i += 2;
      } else {
	dst16[x] = src8[i];
	i += 1;
      }
      
      /* set endianess for the string */
      if (strcmp(dst_encoding, "UTF-16LE") == 0)
	dst16[x] = arch16_2_little16(dst16[x]);
      else
	/* if the byte order is not specified as being little endian then UTF-16 characters are stored big endian */
	dst16[x] = arch16_2_big16 (dst16[x]);
    }

    length = 2 * x;
    src = scratch;
  } else if (encoding_utf16 (src_encoding) && dst_is_utf8) {
    /* UTF-16 - UTF-8/ASCII */
    src16 = (u_int16_t *)src;

    if (strcmp (src_encoding, "UTF-16LE") == 0)
      le = 1;
    else
      /* if the byte order is not specified as being little endian then UTF-16 characters are stored big endian */
      le = -1;

    /* this conversion produces valid UTF-8 but may not produce correct extended ASCII */
    for (i = 0, x = 0 ; x < 1024 && i < src_len/2 && src16[i] != 0 ; i++) {
      if (le == 1)
	src16[i] = little16_2_arch16 (src16[i]);
      else
	src16[i] = big16_2_arch16 (src16[i]);

      if (src16[i] < 0x80) {
	scratch[x++] = src16[i];
      } else if (!dst_is_ascii && src16[i] < 0x800) {
	scratch[x++] = 0xc0 | (src16[i] >> 6);
	scratch[x++] = 0x80 | (src16[i] & 0x3f);
      } else if (!dst_is_ascii) {
	scratch[x++] = 0xe0 | (src16[i] >> 12);
	scratch[x++] = 0x80 | ((src16[i] >> 6) & 0x3f);
	scratch[x++] = 0x80 | (src16[i] & 0x3f);
      } else
	/* character is not in the extended ascii character set */
	scratch[x++] = '_';

      if (le == 1)
	src16[i] = little16_2_arch16 (src16[i]);
      else
	src16[i] = big16_2_arch16 (src16[i]);
    }

    length = x;
    src = scratch;
  } else
    fprintf (stderr, "libupod_convstr: called with an unsupported conversion: %s to %s\n",
	     src_encoding, dst_encoding);

  if (dst_len)
    *dst_len = length;

  *dst = calloc (length + ((dst_is_utf8) ? 1 : 0), 1);

  /* copy converted string into destintion pointer */
  memcpy (*dst, src, length);
}
#endif

/* 
   to_unicode_hack:

   this hack is needed for older iPods to play songs which have non-ascii characters in their filename.
   NOTE: src_encoding parameter is ignored
*/
void to_unicode_hack (u_int16_t **dst, size_t *dst_len, u_int8_t *src, size_t src_len, char *src_encoding) {
  int i, j;
  u_int8_t *dst8;

  if (dst == NULL || dst_len == NULL || src == NULL | src_len == 0)
    return;

  *dst = (u_int16_t *)calloc (1, src_len * 2);
  dst8 = (u_int8_t *)*dst;

  for (i = 0, j = 0 ; i < src_len ; i++) {
    dst8[j++] = src[i];

    if (!(src[i] & 0x80))
      j++;
  }

  *dst = realloc (*dst, j);
  *dst_len = j;
}
