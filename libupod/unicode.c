/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.2 unicode.c
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

#if defined(HAVE_LIBICONV)
#if !defined(const)
#define const /* get rid of const (useless in c anyway?) */
#endif

#include <iconv.h>

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
  u_int8_t scratch[512];
  u_int16_t *dst16, *src16;
  u_int8_t *src8;
  int i, x;
  int le = 0;

  if (encoding_equiv (src_encoding, dst_encoding)) {
    if (dst_len)
      *dst_len = src_len;

    if (encoding_equiv (src_encoding, "UTF-8"))
      *dst = calloc (src_len + 1, 1);
    else
      *dst = calloc (src_len, 1);

    memcpy (*dst, src, src_len);
  } else if (encoding_equiv (src_encoding, "UTF-8") && encoding_utf16 (dst_encoding)) {
    /* UTF-8/ASCII to UTF-16 */
    dst16 = (u_int16_t *)scratch;
    src8  = (u_int8_t *)src;

    if (strcmp(dst_encoding, "UTF-16LE") == 0)
      le = 1;
    else if (strcmp (dst_encoding, "UTF-16BE") == 0)
      le = -1;
    
    for (i = 0, x = 0 ; i < src_len && src8[i] != '\0' ; x++) {
      if ((src8[i] & 0xe0) == 0xe0 && (i + 2 < src_len) && (src8[i+1] & 0xc0) == 0x80 &&
	  (src8[i+2] & 0xc0) == 0x80) {
	dst16[x] = (src8[i+2] & 0x3f) | ((src8[i+1] & 0x3f) << 6) | ((src8[i] & 0x0f) << 12);
	i += 3;
      } else if ((src8[i] & 0xc0) == 0xc0 && (i + 1 < src_len) && (src8[i+1] & 0xc0) == 0x80) {
	dst16[x] = (src8[i+1] & 0x3f) | ((src8[i] & 0x1f) << 6);
	i += 2;
      } else {
	dst16[x] = src8[i];
	i++;
      }
      
      /* correct the endianess of the string if needbe else keep it in the machine's
	 endianess */
      if (le == 1)
	dst16[x] = arch16_2_little16(dst16[x]);
      else if (le == -1)
	dst16[x] = arch16_2_big16 (dst16[x]);
    }

    /* copy converted string into destintion pointer */
    *dst_len = 2 * x;
    *dst = calloc (x, 2);
    if (*dst == NULL) {
      perror ("libupod_convstr|calloc");
      *dst_len = 0;
      return;
    }

    memcpy (*dst, scratch, 2 * x);
  } else if (encoding_utf16 (src_encoding) && encoding_equiv (dst_encoding, "UTF-8")) {
    int dst_isascii = 0;
    /* UTF-16 - UTF-8/ASCII */
    src16 = (u_int16_t *)src;

    if (strcmp (src_encoding, "UTF-16LE") == 0)
      le = 1;
    else if (strcmp (src_encoding, "UTF-16BE") == 0)
      le = -1;

    if (strcmp (dst_encoding, "UTF-8") != 0)
      dst_isascii = 1;

    for (i = 0, x = 0 ; i < src_len/2 && src16[i] != 0 ; i++) {
      if (le == 1)
	src16[i] = little16_2_arch16 (src16[i]);
      else if (le == -1)
	src16[i] = big16_2_arch16 (src16[i]);

      if (src16[i] < 0x80) {
	scratch[x++] = src16[i];
      } else if (!dst_isascii && src16[i] < 0x800) {
	scratch[x++] = 0xc0 | (src16[i] >> 6);
	scratch[x++] = 0x80 | (src16[i] & 0x3f);
      } else if (!dst_isascii) {
	scratch[x++] = 0xe0 | (src16[i] >> 12);
	scratch[x++] = 0x80 | ((src16[i] >> 6) & 0x3f);
	scratch[x++] = 0x80 | (src16[i] & 0x3f);
      } else
	/* character is not in the extended ascii character set */
	scratch[x++] = '_';

      if (le == 1)
	src16[i] = little16_2_arch16 (src16[i]);
      else if (le == -1)
	src16[i] = big16_2_arch16 (src16[i]);
    }

    /* copy converted string into destintion pointer */
    *dst = calloc (x + 1, 1);
    memcpy (*dst, scratch, x);

    if (dst_len)
      *dst_len = x;
  } else
    fprintf (stderr, "libupod_convstr: called with an unsupported encoding: %s to %s\n",
	     src_encoding, dst_encoding);
}
#endif

/* this hack is needed for older iPods to play songs which have non-ascii
   characters in their filename */
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

void path_to_utf8 (u_int8_t **dst, size_t *dst_len, u_int16_t *src,
		      size_t src_len) {
  int i, j;
  u_int8_t *src8 = (u_int8_t *)src;

  if ((dst == NULL) | (dst_len == NULL) | (src == NULL) | (src_len < 1))
    return;

  *dst = calloc (src_len + 1, 1);

  for (i = 0, j = 0 ; i < src_len ; i++) {
    if (src8[i] != '\0')
      (*dst)[j++] = src8[i];
  }

  *dst = realloc (*dst, j);
  *dst_len = j-1;
}

