/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1beta1 image_list.c
 *
 *   Functions to manipulate the list of artwork in an ArtworkDB.
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
#include <sys/stat.h>

#include <sys/uio.h>
#include <unistd.h>

#include <fcntl.h>

#include <errno.h>

#include <wand/magick_wand.h>

#include "itunesdbi.h"

static iihm_t *db_iihm_fill (tree_node_t *iihm_header);

int db_thumb_add (ipoddb_t *photodb, MagickWand *magick_wand, int iihm_identifier) {
  tree_node_t *dshm_header;
  tree_node_t *dohm_header, *inhm_header, *iihm_header;

  struct db_iihm *iihm_data;
  
  char file_name[255];
  char file_name_mac[255];
  unsigned long file_id;

  int ret;

  if ((photodb == NULL) || (magick_wand == NULL) || (iihm_identifier < 1))
    return -EINVAL;

  db_log (photodb, 0, "db_thumb_add: entering...\n");

  /* find the image list */
  if ((ret = db_iihm_retrieve (photodb, &iihm_header, &dshm_header, iihm_identifier)) < 0) {
    db_log (photodb, ret, "db_thumb_add: could not retrieve iihm header\n");

    return ret;
  }

  if (MagickGetImageWidth(magick_wand) == 56)
    file_id = 1017;
  else if (MagickGetImageWidth(magick_wand) == 140)
    file_id = 1016;
   
  sprintf (file_name, "F%lu_1.ithmb", file_id);

  sprintf (file_name_mac, ":F%lu_1.ithmb", file_id);

  db_dohm_create_generic (&dohm_header, 0x18, 0x02);
  db_inhm_create (&inhm_header, file_id, file_name, file_name_mac, magick_wand);

  db_fihm_register (photodb, file_name, file_id);

  db_attach (dohm_header, inhm_header);
  db_attach (iihm_header, dohm_header);

  iihm_data = (struct db_iihm *)iihm_header->data;
  iihm_data->num_thumbs++;

  return 0;
}

int db_photo_add (ipoddb_t *photodb, unsigned char *image_data, size_t image_size) {
  tree_node_t *dshm_header, *new_iihm_header;
  struct db_ilhm *ilhm_data;
  struct db_dfhm *dfhm_data;

  MagickWand *magick_wand;
  PixelWand *pixel_wand;
  int identifier, id1, id2, ret;
  int image_height, image_width;
  int scale_height, scale_width;
  int border_height, border_width;

  if ((photodb == NULL) || (image_data == NULL) || (image_size < 1))
    return -EINVAL;

  db_log (photodb, 0, "db_photo_add: entering...\n");

  /* find the image list */
  if ((ret = db_dshm_retrieve (photodb, &dshm_header, 1)) < 0) {
    db_log (photodb, ret, "Could not get image list header\n");
    return ret;
  }

  dfhm_data = (struct db_dfhm *)photodb->tree_root->data;

  id1 = id2 = identifier = dfhm_data->next_iihm;

  if ((ret = db_iihm_create (&new_iihm_header, identifier, id1, id2)) < 0) {
    db_log (photodb, ret, "Could not create iihm entry\n");
    return ret;
  }

  new_iihm_header->parent = dshm_header;
  db_attach (dshm_header, new_iihm_header);

  /* everything was successfull, increase the image count in the ilhm header */
  ilhm_data = (struct db_ilhm *)dshm_header->children[0]->data;
  ilhm_data->num_images += 1;

  dfhm_data->next_iihm++;

  db_log (photodb, 0, "db_photo_add: complete. Creating default thumbnails..\n");

  /* Make thumbnails and add them to the database */
  magick_wand = NewMagickWand ();
  pixel_wand  = NewPixelWand ();

  ret = MagickReadImageBlob (magick_wand, image_data, image_size);
  if (ret == MagickFalse) {
    DestroyMagickWand (magick_wand);

    return -1;
  }

  image_height = MagickGetImageHeight (magick_wand);
  image_width  = MagickGetImageWidth (magick_wand);

  if (image_width > image_height) {
    scale_width  = 140;
    scale_height = (int) (140.0 * (float) image_height/(float) image_width)/2 * 2;
  } else {
    scale_width  = (int) (140.0 * (float) image_width/(float) image_height)/2 * 2;
    scale_height = 140;
  }

  border_height = (140 - scale_height)/2;
  border_width  = (140 - scale_width)/2;
  
  MagickResizeImage (magick_wand, scale_width, scale_height, LanczosFilter, 0.9);
  MagickBorderImage (magick_wand, pixel_wand, border_width, border_height);
  db_thumb_add (photodb, magick_wand, identifier);
  DestroyMagickWand (magick_wand);


  if (image_width > image_height) {
    scale_width  = 56;
    scale_height = (int) (56.0 * (float) image_height/(float) image_width)/2 * 2;
  } else {
    scale_width  = (int) (56.0 * (float) image_width/(float) image_height)/2 * 2;
    scale_height = 56;
  }

  border_height = (56 - scale_height)/2;
  border_width  = (56 - scale_width)/2;

  magick_wand = NewMagickWand();
  ret = MagickReadImageBlob (magick_wand, image_data, image_size);
  MagickResizeImage (magick_wand, scale_width, scale_height, LanczosFilter, 0.9);
  MagickBorderImage (magick_wand, pixel_wand, border_width, border_height);
  db_thumb_add (photodb, magick_wand, identifier);


  DestroyMagickWand (magick_wand);
  DestroyPixelWand (pixel_wand);

  return identifier;
}

int db_photo_remove (ipoddb_t *photodb, u_int32_t identifier) {

  return -1;
}

int db_photo_list (ipoddb_t *artworkdb, GList **head) {
  tree_node_t *dshm_header, *iihm_header, *ilhm_header;
  struct db_ilhm *ilhm_data;

  int i, ret, *iptr;

  if (head == NULL || artworkdb == NULL)
    return -EINVAL;

  *head = NULL;

  if ((ret = db_dshm_retrieve (artworkdb, &dshm_header, 0x01)) < 0)
    return ret;

  ilhm_header = dshm_header->children[0];
  ilhm_data = (struct db_ilhm *)ilhm_header->data;

  if (ilhm_data->num_images == 0)
    return -1;

  for (i = dshm_header->num_children - 1 ; i > 0 ; i--) {
    iihm_header = dshm_header->children[i];
    iptr = (int *)iihm_header->data;

    /* only add tree nodes containing tihm entries
       to the new song list */
    if (iptr[0] != IIHM)
      continue;

    *head = g_list_prepend (*head, db_iihm_fill (iihm_header));
  }

  return 0;
}

void db_photo_list_free (GList **head) {
  GList *tmp;

  for (tmp = g_list_first (*head) ; tmp ; tmp = g_list_next (tmp))
    iihm_free ((iihm_t *)(tmp->data));

  g_list_free (*head);

  *head = NULL;
}

static dohm_t *db_dohm_fill_art (tree_node_t *entry) {
  dohm_t *dohms;
  struct db_dohm *dohm_data;
  tree_node_t **dohm_list;
  int i, *iptr;

  dohms = (dohm_t *) calloc (entry->num_children, sizeof(dohm_t));

  dohm_list = entry->children;

  for (i = 0 ; i < entry->num_children ; i++) {
    dohm_data = (struct db_dohm *)dohm_list[i]->data;

    iptr = (int *)dohm_list[i]->data;
    
    dohms[i].type = dohm_data->type;
    dohms[i].size = iptr[6];
    dohms[i].data = (u_int16_t *) calloc (dohms[i].size, 1);
    memcpy(dohms[i].data, &(dohm_list[i]->data[0x24]), dohms[i].size);
  }

  return dohms;
}

static iihm_t *db_iihm_fill (tree_node_t *iihm_header) {
  tree_node_t *inhm_header;

  iihm_t *iihm;

  struct db_iihm *iihm_data;
  struct db_inhm *inhm_data;

  int i;

  iihm = (iihm_t *) calloc (1, sizeof (iihm_t));
  if (iihm == NULL) {
    perror ("db_iihm_fill|calloc");

    return NULL;
  }

  iihm_data = (struct db_iihm *)iihm_header->data;

  iihm->identifier = iihm_data->identifier;
  iihm->num_inhm  = iihm_data->num_thumbs;

  iihm->inhms      = (inhm_t *) calloc (iihm->num_inhm, sizeof (inhm_t));

  for (i = 0 ; i < iihm->num_inhm ; i++) {
    inhm_header = iihm_header->children[i]->children[0];
    inhm_data   = (struct db_inhm *)inhm_header->data;

    iihm->inhms[i].file_offset = inhm_data->file_offset;
    iihm->inhms[i].image_size  = inhm_data->image_size;
    iihm->inhms[i].height      = inhm_data->height;
    iihm->inhms[i].width       = inhm_data->width;

    iihm->inhms[i].num_dohm    = inhm_data->num_dohm;
    
    iihm->inhms[i].dohms       = db_dohm_fill_art (inhm_header);
  }

  return iihm;
}

void inhm_free (inhm_t *inhm) {
  if (inhm == NULL)
    return;

  dohm_free (inhm->dohms, inhm->num_dohm);
}

void iihm_free (iihm_t *iihm) {
  int i;

  if (iihm == NULL)
    return;

  for (i = 0 ; i < iihm->num_inhm ; i++)
    inhm_free (&iihm->inhms[i]);

  free (iihm->inhms);
}
