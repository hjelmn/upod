/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.2 image_list.c
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
#include <libgen.h>
#include <fcntl.h>

#include <errno.h>

#include <wand/magick_wand.h>

#include "itunesdbi.h"

static iihm_t *db_iihm_fill (tree_node_t *iihm_header);

/* The image described by image_data will be scaled to the correct size */
int db_thumb_add (ipoddb_t *photodb, int iihm_identifier, unsigned char *image_data,
		  size_t image_size, size_t thumb_width, size_t thumb_height) {
  tree_node_t *dshm_header;
  tree_node_t *dohm_header, *inhm_header, *iihm_header;

  struct db_iihm *iihm_data;
  
  char file_name[255];
  char file_name_mac[255];
  unsigned long file_id;
  unsigned long image_height, image_width;
  unsigned long scale_width, scale_height;
  unsigned long border_height, border_width;

  char *tmp;

  MagickWand *magick_wand;
  PixelWand *pixel_wand;

  int ret;

  if ((photodb == NULL) || (iihm_identifier < 1))
    return -EINVAL;

  db_log (photodb, 0, "db_thumb_add: entering...\n");

  /* find the image list */
  if ((ret = db_iihm_retrieve (photodb, &iihm_header, &dshm_header, iihm_identifier)) < 0) {
    db_log (photodb, ret, "db_thumb_add: could not retrieve iihm header\n");

    return ret;
  }

  /* Make thumbnails and add them to the database */
  magick_wand = NewMagickWand ();
  pixel_wand  = NewPixelWand ();

  PixelSetColor (pixel_wand, "black");

  ret = MagickReadImageBlob (magick_wand, image_data, image_size);
  if (ret == MagickFalse) {
    DestroyMagickWand (magick_wand);

    fprintf (stderr, "******* Error reading image blob *******\n");

    return -1;
  }

  image_height = MagickGetImageHeight (magick_wand);
  image_width  = MagickGetImageWidth (magick_wand);

  if (image_width > image_height) {
    scale_width  = thumb_width;
    scale_height = (int) ((float)thumb_width * (float) image_height/(float) image_width)/2 * 2;
  } else {
    scale_width  = (int) ((float)thumb_height * (float) image_width/(float) image_height)/2 * 2;
    scale_height = thumb_height;
  }

  border_width  = (thumb_width - scale_width)/2;
  border_height = (thumb_height - scale_height)/2;
  
  MagickResizeImage (magick_wand, scale_width, scale_height, LanczosFilter, 0.9);
  MagickBorderImage (magick_wand, pixel_wand, border_width, border_height);

  if (thumb_width == 56)
    file_id = 1017;
  else if (thumb_width == 140)
    file_id = 1016;
  else if (thumb_width == 640)
    file_id = 1019;
  else
    file_id = 1015;


  tmp = strdup (photodb->path);
  sprintf (file_name, "%s/F%lu_1.ithmb", dirname (tmp), file_id);
  free (tmp);

  sprintf (file_name_mac, ":F%lu_1.ithmb", file_id);

  db_dohm_create_generic (&dohm_header, 0x18, 0x02);
  db_inhm_create (&inhm_header, file_id, file_name, file_name_mac, magick_wand);

  db_fihm_register (photodb, file_name, file_id);

  db_attach (dohm_header, inhm_header);
  db_attach (iihm_header, dohm_header);

  DestroyMagickWand (magick_wand);
  DestroyPixelWand (pixel_wand);

  iihm_data = (struct db_iihm *)iihm_header->data;
  iihm_data->num_thumbs++;

  return 0;
}

static int iihm_lookup (ipoddb_t *photodb, int id1, int id2) {
  tree_node_t *dshm_header;
  int i, ret;
  struct db_ilhm *ilhm_data;

  if (photodb == NULL)
    return -EINVAL;

  db_log (photodb, 0, "iihm_lookup: entering...\n");

  /* find the image list */
  if ((ret = db_dshm_retrieve (photodb, &dshm_header, 1)) < 0) {
    db_log (photodb, ret, "Could not get image list header\n");
    return ret;
  }

  ilhm_data = (struct db_ilhm *)dshm_header->children[0]->data;

  for (i = 0 ; i < ilhm_data->num_images ; i++) {
    struct db_iihm *iihm_data = (struct db_iihm *)dshm_header->children[i + 1]->data;

    if (iihm_data->id1 == id1 && iihm_data->id2 == id2)
      return 1;
  }

  return 0;
}

int db_photo_add (ipoddb_t *photodb, unsigned char *image_data, size_t image_size, int id1, int id2) {
  tree_node_t *dshm_header, *new_iihm_header;
  struct db_ilhm *ilhm_data;
  struct db_dfhm *dfhm_data;

  int identifier, ret;

  if ((photodb == NULL) || (image_data == NULL) || (image_size < 1))
    return -EINVAL;

  db_log (photodb, 0, "db_photo_add: entering...\n");

  /* find the image list */
  if ((ret = db_dshm_retrieve (photodb, &dshm_header, 1)) < 0) {
    db_log (photodb, ret, "Could not get image list header\n");
    return ret;
  }

  if (iihm_lookup (photodb, id1, id2)) {
    db_log (photodb, ret, "Image already exists in the database\n");

    return 0;
  }

  dfhm_data = (struct db_dfhm *)photodb->tree_root->data;

  identifier = dfhm_data->next_iihm;

  if ((ret = db_iihm_create (&new_iihm_header, identifier, id1, id2)) < 0) {
    db_log (photodb, ret, "Could not create iihm entry\n");
    return ret;
  }

  new_iihm_header->parent = dshm_header;
  db_attach (dshm_header, new_iihm_header);

  db_log (photodb, 0, "db_photo_add: complete. Creating default thumbnails..\n");

  if (db_thumb_add (photodb, identifier, image_data, image_size, 56, 56) < 0) {
    db_log (photodb, -1, "Could not create default thumbnails.\n");

    db_detach (dshm_header, dshm_header->num_children-1, &new_iihm_header);
    db_free_tree (new_iihm_header);
    
    return -1;
  }

  db_thumb_add (photodb, identifier, image_data, image_size, 140, 140);

  /* everything was successfull, increase the image count in the ilhm header */
  ilhm_data = (struct db_ilhm *)dshm_header->children[0]->data;
  ilhm_data->num_images += 1;

  dfhm_data->next_iihm++;

  db_log (photodb, 0, "db_photo_add: completed...\n");

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
  iihm->id1        = iihm_data->id1;
  iihm->id2        = iihm_data->id2;
  iihm->num_inhm   = iihm_data->num_thumbs;

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
