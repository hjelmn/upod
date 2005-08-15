/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 inhm.c
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
#include <fcntl.h>

#include <errno.h>

#include <wand/magick_wand.h>
#include <sys/stat.h>

#include "itunesdbi.h"

#define INHM_HEADER_SIZE 0x4c

static u_int16_t host16_to_little (u_int16_t x) {
  return (x & 0x00ff) << 8 | (x & 0xff00) >> 8;
}

#if defined(HAVE_LIBWAND)
int db_inhm_create (tree_node_t **entry, int file_id, char *file_name,
		    char *rel_mac_path, MagickWand *magick_wand) {
  struct db_inhm *inhm_data;
  tree_node_t *dohm_header;
  int ret;
  FILE *fh;
  unsigned long num_wands = 1;
  unsigned int r, g, b, i, j;

  dohm_t dohm;

  PixelIterator *pixel_iterator;
  PixelWand **pixel_wands;

  if ((ret = db_node_allocate (entry, INHM, INHM_HEADER_SIZE, INHM_HEADER_SIZE)) < 0)
    return ret;

  inhm_data = (struct db_inhm *)(*entry)->data;
  inhm_data->file_id    = file_id;

  inhm_data->height     = MagickGetImageHeight(magick_wand);
  inhm_data->width      = MagickGetImageWidth(magick_wand);

  /* Thumbnails are 16 bit rgb images (2 Bpp) */
  inhm_data->image_size = inhm_data->height * inhm_data->width * 2;
  fh = fopen (file_name, "a");

  if (fh == NULL)
    return -errno;

  inhm_data->file_offset = ftell (fh);

  /* Write the thumbnail to the thumbnail file */
  MagickResetIterator(magick_wand);
  pixel_iterator = NewPixelIterator (magick_wand);

  j = 0;

  while (num_wands != 0) {
    u_int16_t pixel;

    pixel_wands = PixelGetNextIteratorRow (pixel_iterator, &num_wands);

    for (i = 0 ; i < num_wands ; i++) {
      r = (int)(PixelGetRed (pixel_wands[i]) * 31.0);
      g = (int)(PixelGetGreen (pixel_wands[i]) * 63.0);
      b = (int)(PixelGetBlue (pixel_wands[i]) * 31.0);
      
      pixel = 0x07c0;
      pixel = b | (g << 5) | (r << 11);
      pixel = host16_to_little (pixel);

      (void)fwrite (&pixel, 2, 1, fh);
    }

    j++;
  }
  
  (void)fclose (fh);

  dohm.data = rel_mac_path;
  dohm.type = 3;
  (void)db_dohm_create (&dohm_header, dohm, 12, 0);
  
  (void)db_attach (*entry, dohm_header);
  
  inhm_data = (struct db_inhm *)(*entry)->data;
  inhm_data->num_dohm++;
  
  pixel_iterator = DestroyPixelIterator (pixel_iterator);

  return 0;
}
#endif
