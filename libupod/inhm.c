/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 inhm.c
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

#include <sys/types.h>
#include <fcntl.h>

#include <wand/magick_wand.h>
#include <sys/stat.h>

#include "itunesdbi.h"

#define INHM_HEADER_SIZE 0x4c

static u_int16_t host16_to_little (u_int16_t x) {
  return (x & 0x00ff) << 8 | (x & 0xff00) >> 8;
}

int db_inhm_create (tree_node_t **entry, int file_id, char *file_name, MagickWand *magick_wand) {
  struct db_inhm *inhm_data;
  int ret;
  struct stat statinfo;
  int fd;
  unsigned long num_wands;
  int r, g, b, i;

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
  
  if (stat (file_name, &statinfo) < 0) {
    fd = open (file_name, O_WRONLY | O_CREAT | O_TRUNC);
    inhm_data->file_offset = 0;
  } else {
    fd = open (file_name, O_WRONLY | O_APPEND);
    inhm_data->file_offset = statinfo.st_size;
  }

  pixel_iterator = NewPixelIterator (magick_wand);

  while (pixel_wands = PixelGetNextIteratorRow (pixel_iterator, &num_wands)) {
    u_int16_t pixel;

    for (i = 0 ; i < num_wands ; i++) {
      r = (int)(PixelGetRed (pixel_wands[i]) * 15.0);
      g = (int)(PixelGetGreen (pixel_wands[i]) * 15.0);
      b = (int)(PixelGetBlue (pixel_wands[i]) * 255.0);
      pixel = (r << 12) | (g << 8) | b;

      pixel = host16_to_little (pixel);

      write (fd, &pixel, 2);
    }
    
    DestroyPixelWands (pixel_wands, num_wands);
  }

  DestroyPixelIterator (pixel_iterator);

  return 0;
}
