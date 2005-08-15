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
#if !defined(const)
#define const /* get rid of const (useless in c anyway?) */
#endif

#if defined(HAVE_LIBICONV)
#include <iconv.h>

void to_utf8 (u_int8_t **dst, u_int8_t *src, size_t src_len, char *encoding) {
  iconv_t conv;
  size_t final_size;
  char *inbuf, *outbuf;
  size_t inbytes, outbytes;
  
  if (dst == NULL)
    return;

  if (src_len == 0) {
    *dst = NULL;
    return;
  }

  inbytes = src_len;
  inbuf   = (char *)src;

  /* Allocate plenty of space for the conversion */
  outbytes = src_len * 3;
  *dst = outbuf  = calloc (outbytes + 1, 1);

  conv = iconv_open ("UTF-8", encoding);

  iconv (conv, &inbuf, &inbytes, &outbuf, &outbytes);

  iconv_close (conv);
 
  final_size = (src_len * 3) - outbytes;

  *dst = realloc (*dst, final_size + 1);
  (*dst)[final_size] = '\0';
}

/* Converts the input from UTF8/ASCII to Unicode */
void to_unicode (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
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
/* only works with ASCII and UTF-16 input */
void to_utf8 (u_int8_t **dst, u_int8_t *src, size_t src_len, char *encoding) {
  if (strcmp (encoding, "ASCII") == 0 || strcmp (encoding, "UTF-8") == 0 ||
      strcmp (encoding, "ISO-8859-1") == 0) {
    /* ASCII - ASCII (easy) */
    *dst = calloc (src_len + 1, 1);
    memcpy (*dst, src, src_len);
  } else if (strncmp (encoding, "UTF-16", 6) == 0) {
    int i, x;
    u_int16_t *src16;
    u_int8_t scratch[512];

    src16 = (u_int16_t *)src;

    /* UTF-16 - ASCII (still easy) */
    for (i = 0, x = 0 ; i < src_len/2 ; i++) {
      if (src16[i] < 0x80) {
	scratch[x++] = src16[i];
      } else if (src16[i] < 0x800) {
	scratch[x++] = 0xc0 & (src16[i] >> 6);
	scratch[x++] = 0x80 & (src16[i] & 0x3f);
      } else {
	scratch[x++] = 0xe0 & (src16[i] >> 12);
	scratch[x++] = 0x80 & ((src16[i] >> 6) & 0x3f);
	scratch[x++] = 0x80 & (src16[i] & 0x3f);
      }
    }

    *dst = calloc (x + 1, 1);
    memcpy (*dst, scratch, x);
  } else {
    fprintf (stderr, "to_utf8: called with an unsupported encoding.\n");
    *dst = NULL;
  }

}

void to_unicode (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
		 size_t src_len, char *src_encoding, char *dst_encoding) {
  if (strcmp (src_encoding, dst_encoding) == 0) {
    *dst_len = src_len;
    *dst = calloc (1, src_len);
    memcpy (*dst, src, src_len);
  } else {
    u_int16_t *dst16;
    int i, x;

    u_int8_t scratch[512];

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
    *dst = calloc (x, 2);
    if (*dst == NULL) {
      perror ("unicode_check_and_copy|calloc");
      *dst_len = 0;
      return;
    }

    memcpy (*dst, scratch, 2*x);
  }
}
#endif

/* this hack is needed to play songs with unicode filenames on older ipods */
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
