/**
 *   (c) 2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.4.0 itunessd.c
 *
 *   Routines for reading/writing the iPod's databases.
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

#ifndef S_SPLINT_S
#include <unistd.h>
#endif

#include <fcntl.h>

#include <errno.h>

#include "itunesdbi.h"

int get_uint24 (unsigned char *buf, int block) {
  int x;

  x = (buf[block * 3 + 0] << 16) | (buf[block * 3 + 1] << 8) | buf[block * 3 + 2];
  
  return x;
}

static void set_uint24 (unsigned char *buf, int block, int value) {
  buf[block * 3 + 0] = (value & 0xff0000) >> 16;
  buf[block * 3 + 1] = (value & 0x00ff00) >> 8;
  buf[block * 3 + 2] = value & 0x0000ff;
}

static void inc_uint24 (unsigned char *buf, int block) {
  int value;

  value = get_uint24 (buf, block);
  set_uint24 (buf, block, ++value);
}

static void dec_uint24 (unsigned char *buf, int block) {
  int value;

  value = get_uint24 (buf, block);
  set_uint24 (buf, block, --value);
}

int sd_load (ipoddb_t *ipod_sd, char *path, int flags) {
  int iPod_SD_fd;
  struct stat statinfo;

  if (ipod_sd == NULL || path == NULL)
    return -EINVAL;

  db_log (ipod_sd, 0, "sd_load: entering...\n");

  if (stat(path, &statinfo) < 0) {
    db_log (ipod_sd, errno, "sd_load|stat: %s\n", strerror(errno));

    return -errno;
  }

  if (!S_ISREG(statinfo.st_mode)) {
    db_log (ipod_sd, -1, "sd_load: %s, not a regular file\n", path);

    return -EINVAL;
  }


  if (statinfo.st_size < 18) {
    db_log (ipod_sd, -1, "sd_load: %s is too small to be an iTunesSD file.\n");

    return -EINVAL;
  }

  iPod_SD_fd = open (path, O_RDONLY);

  if (iPod_SD_fd < 0) {
    db_log (ipod_sd, errno, "sd_load: could not open the iTunesSD %s\n", path);

    return -errno;
  }

  ipod_sd->tree_root = calloc (1, sizeof (struct tree_node));
  
  if (ipod_sd->tree_root == NULL) {
    close (iPod_SD_fd);

    perror ("sd_load|calloc");

    return -errno;
  }

  ipod_sd->tree_root->data = calloc (1, statinfo.st_size);

  if (ipod_sd->tree_root->data == NULL) {
    free (ipod_sd->tree_root);
    close (iPod_SD_fd);

    perror ("sd_load|calloc");

    return -errno;
  }


  if (read (iPod_SD_fd, ipod_sd->tree_root->data, statinfo.st_size) < statinfo.st_size) {
    free (ipod_sd->tree_root);
    close (iPod_SD_fd);

    db_log (ipod_sd, -1, "sd_load: short read while reading from the iTunesSD file %s\n", path);

    return -1;
  }

  ipod_sd->tree_root->data_size = statinfo.st_size;

  close (iPod_SD_fd);

  ipod_sd->path = strdup (path);
  ipod_sd->type = 2;

  return 0;
}

int sd_song_add (ipoddb_t *ipod_sd, char *ipod_path, int start, int stop, int volume) {
  int file_type;
  unsigned char *sd_data, *song_offset;
  int sd_size;
  u_int16_t *unicode_data;
  size_t unicode_length;

  if (ipod_sd == NULL || ipod_path == NULL || ipod_path == NULL)
    return -EINVAL;

  if (ipod_sd->tree_root == NULL || ipod_sd->tree_root->data == NULL) {
    db_log (ipod_sd, -1, "sd_song_add: iTunesSD not loaded.\n");

    return -1;
  }

  db_log (ipod_sd, 0, "sd_song_add: entering...\n");
  db_log (ipod_sd, 0, "sd_song_add: adding file %s to database.\n", ipod_path);

  to_unicode (&unicode_data, &unicode_length, ipod_path, strlen (ipod_path), "UTF-8", "UTF-16BE");

  if (db_lookup (ipod_sd, IPOD_PATH, ipod_path) > -1) {
    /* Future. Check modification date of file vs. database. */
    db_log (ipod_sd, 0, "sd_song_add: file already exists in database.\n");
    
    return -1; /* A song already exists in the database with this path */
  }

  if (strncasecmp (ipod_path + (strlen(ipod_path) - 3), "mp3", 3) == 0)
    file_type = 1;
  else if ( (strncasecmp (ipod_path + (strlen(ipod_path) - 3), "m4a", 3) == 0)  ||
	    (strncasecmp (ipod_path + (strlen(ipod_path) - 3), "m4p", 3) == 0)  ||
	    (strncasecmp (ipod_path + (strlen(ipod_path) - 3), "aac", 3) == 0) )
    file_type = 2;
  else if (strncasecmp (ipod_path + (strlen(ipod_path) - 3), "mp3", 3) == 0)
    file_type = 3;
  else {
    db_log (ipod_sd, -1, "sd_song_add: file type is not supported or incorrect extension.\n");

    return -1;
  }

  sd_size = ipod_sd->tree_root->data_size + 0x00022e;
  sd_data = realloc (ipod_sd->tree_root->data, sd_size);

  if (sd_data == NULL) {
    perror ("sd_song_add|realloc");

    return -errno;
  }

  song_offset = &sd_data[sd_size - 0x00022e];
  memset (song_offset, 0, 0x00022e);

  set_uint24 (song_offset, 0, 0x00022e);
  set_uint24 (song_offset, 1, 0x5aa501);

  /* start time and stop time are not yet supported */
  set_uint24 (song_offset, 2, 0); /* start time */
  set_uint24 (song_offset, 5, 0); /* stop time */
 

  set_uint24 (song_offset, 8, volume + 100);
  set_uint24 (song_offset, 9, file_type);
  set_uint24 (song_offset, 10, 0x000200);

  memcpy(&song_offset[32], unicode_data, unicode_length);

  song_offset[555] = 0x01; /* song will be included in shuffle */
  song_offset[556] = 0x00; /* no bookmark support at this time */
  song_offset[557] = 0x00; /* unknown */

  inc_uint24 (sd_data, 0);
  
  ipod_sd->tree_root->data = sd_data;
  ipod_sd->tree_root->data_size = sd_size;

  free (unicode_data);

  db_log (ipod_sd, 0, "sd_song_add: complete\n");

  return 0;
}

int sd_song_remove (ipoddb_t *ipod_sd, int number) {
  int header_size;
  int sd_size;
  unsigned char *sd_data;

  if (ipod_sd == NULL || number < 0)
    return -EINVAL;

  db_log (ipod_sd, 0, "sd_song_remove: entering...\n");

  return -1;

  sd_data = ipod_sd->tree_root->data;
  sd_size = ipod_sd->tree_root->data_size;

  header_size = get_uint24 (sd_data, 3);

  if ((number * 0x00022e) < (sd_size - header_size)) {
    if ((number + 1) * 0x00022e < (sd_size - header_size)) {
      unsigned char *tmp, *tmp2;

      tmp  = &ipod_sd->tree_root->data[header_size + number * 0x00022e];
      tmp2 = &ipod_sd->tree_root->data[header_size + (number + 1) * 0x00022e];

      memmove (tmp, tmp2, 0x00022e);
    }

    /* decrease the song count */
    dec_uint32 (sd_data, 0);
    
    sd_size -= 0x00022e;
    ipod_sd->tree_root->data_size = sd_size;

    sd_data = realloc (sd_data, sd_size);

    if (sd_data == NULL) {
      perror ("sd_song_remove|realloc");

      return -errno;
    }

    ipod_sd->tree_root->data = sd_data;
  }

  db_log (ipod_sd, 0, "sd_song_remove: complete\n");

  return 0;
}

int sd_create (ipoddb_t *ipod_sd, u_int8_t *path, int flags) {
  if (ipod_sd == NULL)
    return -EINVAL;

  ipod_sd->tree_root = calloc (1, sizeof (struct tree_node));

  if (ipod_sd->tree_root == NULL) {
    perror ("sd_create|calloc");

    return -errno;
  }

  ipod_sd->tree_root->data = calloc (18, 1);

  if (ipod_sd->tree_root->data == NULL) {
    perror ("sd_create|calloc");

    free (ipod_sd->tree_root);

    ipod_sd->tree_root = NULL;

    return -errno;
  }

  ipod_sd->tree_root->data_size = 18;
  ipod_sd->flags = flags;
  ipod_sd->type  = 2;
  ipod_sd->path  = strdup (path);

  set_uint24 (ipod_sd->tree_root->data, 1, 0x010600);
  set_uint24 (ipod_sd->tree_root->data, 2, 18);

  return 0;
}

int sd_write (ipoddb_t ipod_sd, char *path) {
  int fd;

  int ret;
  int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  if (ipod_sd.tree_root == NULL || path == NULL || ipod_sd.type != 2)
    return -EINVAL;

  db_log (&ipod_sd, 0, "sd_write: entering...\n");

  if ((fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, perms)) < 0) {
    db_log (&ipod_sd, -errno, "sd_write: error opening %s, %s\n", path, strerror (errno));

    return -errno;
  }

  ret = write (fd, ipod_sd.tree_root->data, ipod_sd.tree_root->data_size);
  
  close (fd);

  db_log (&ipod_sd, 0, "sd_write: complete. wrote %i Bytes\n", ret);
  
  return ret;
}

int sd_song_list (ipoddb_t *ipod_sd, GList **head) {
  int i;
  int num_songs;
  int header_size;
  unsigned char *song_list;

  tihm_t *tihm;

  if (head == NULL || ipod_sd == NULL || ipod_sd->type != 2)
    return -EINVAL;

  *head = NULL;

  num_songs = get_uint24 (ipod_sd->tree_root->data, 0);
  header_size = get_uint24 (ipod_sd->tree_root->data, 2);
  song_list = &ipod_sd->tree_root->data[header_size];

  for (i = num_songs-1 ; i >= 0 ; i--) {
    tihm = calloc (1, sizeof (tihm_t));

    tihm->num = i;
    tihm->type = get_uint24 (&song_list[i * 0x00022e], 9);
    tihm->num_dohm = 1;
    tihm->dohms = calloc (1, sizeof (dohm_t));
    tihm->dohms->type = IPOD_PATH;
    to_utf8 (&(tihm->dohms->data), &song_list[i * 0x00022e + 32], 522, "UTF-16BE");

    *head = g_list_prepend (*head, (gpointer)tihm);
  }

  return 0;
}
