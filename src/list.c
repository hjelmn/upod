/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.0 list.c
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

#include "itunesdb.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

void usage (void) {
  printf("Usgae:\n");
  printf(" db_list [--shuffle] <database>\n");

  exit(1);
}

char *str_type(int dohm_type) {
  static char temp_str[30];

  switch (dohm_type) {
  case IPOD_TITLE:
    return "Title";
  case IPOD_PATH:
    return "Path";
  case IPOD_ALBUM:
    return "Album";
  case IPOD_ARTIST:
    return "Artist";
  case IPOD_GENRE:
    return "Genre";
  case IPOD_TYPE:
    return "Type";
  case IPOD_COMMENT:
    return "Comment";
  case IPOD_EQ:
    return "Equilizer";
  case IPOD_COMPOSER:
    return "Composer";
  case IPOD_DESCRIPTION:
    return "Description";
  case IPOD_URL:
    return "URL";
  case IPOD_PODCAST_URL:
    return "PURL";
  case IPOD_CATAGORY:
    return "Catagory";
  case -1:
    return "Error";
  default:
    sprintf (temp_str, "Unknown (%08x)", dohm_type);
    return temp_str;
  }
}

int main (int argc, char *argv[]) {
  db_list_t *songs, *tmp, *tmp2, *list;
  db_list_t *playlists;

  tihm_t *tihm;
  pyhm_t *pyhm;
  iihm_t *iihm;
  inhm_t *inhm;
  
  int i, j;
  
  int ipod_shuffle = 0;

  ipoddb_t itunesdb;

  memset (&itunesdb, 0, sizeof (ipoddb_t));
  db_set_debug (&itunesdb, 5, stderr);

  if (argc == 2 && strcmp (argv[1], "--shuffle") != 0) {
    if (db_load (&itunesdb, argv[1], 0x4) < 0) {
      fprintf (stderr, "itdblist: could not open the iPod database %s\n", argv[1]);

      exit(1);
    }

    if (itunesdb.type == 2)
      ipod_shuffle = 1;
  } else if (argc == 3 && strcmp (argv[1], "--shuffle") == 0) {
    ipod_shuffle = 1;

    if (sd_load (&itunesdb, argv[2], 0x0) < 0) {
      fprintf (stderr, "itdblist: could not open the iPod Shuffle database %s\n", argv[2]);

      exit(1);
    }
  } else
    usage ();

  /* get song lists */
  songs = NULL;

  if (ipod_shuffle == 0)
    db_song_list (&itunesdb, &songs);
  else
    sd_song_list (&itunesdb, &songs);

  if (songs == NULL) {
    fprintf (stdout, "Could not get song list\n");
    
    db_photo_list (&itunesdb, &songs);

    if (songs != NULL) { 
      for (tmp = db_list_first (songs) ; tmp ; tmp = db_list_next (tmp)) {
	iihm = tmp->data;

	fprintf (stdout, "%04x | \n", iihm->identifier);
	fprintf (stdout, " num_thumbs: %i\n", iihm->num_inhm);

	for (j = 0 ; j < iihm->num_inhm ; j++) {
	  inhm = &iihm->inhms[j];
	  
	  fprintf (stdout, "   Thumb  : %i\n", j);
	  fprintf (stdout, "     offset     : %08x\n", inhm->file_offset);
	  fprintf (stdout, "     size       : %08x\n", inhm->image_size);
	  fprintf (stdout, "     dimensions : %ix%i\n", inhm->height, inhm->width);

	  for (i = 0 ; i < inhm->num_dohm ; i++)
	    fprintf (stdout, "     %-10s : %s\n", "Filename", inhm->dohms[i].data);
	  
	  fprintf (stdout, "\n");
	}
      }
      
      db_photo_list_free (&songs);
    } else
      fprintf (stdout, "Could not get image list\n");

  } else
    /* dump songlist contents */
    for (tmp = db_list_first (songs) ; tmp ; tmp = db_list_next (tmp)) {
      tihm = tmp->data;
      
      fprintf (stdout, "%04i |\n", tihm->num);
      
      fprintf (stdout, " encoding: %d\n", tihm->bitrate);
      fprintf (stdout, " length  : %03i:%02i.%03i\n", tihm->time/60000, (tihm->time % 60000)/1000, tihm->time % 1000);
      fprintf (stdout, " type    : %d\n", tihm->type);
      fprintf (stdout, " num dohm: %d\n", tihm->num_dohm);
      fprintf (stdout, " samplert: %d\n", tihm->samplerate);
      fprintf (stdout, " stars   : %d\n", tihm->stars);
      fprintf (stdout, " year    : %d\n", tihm->year);
      fprintf (stdout, " release : %s\n", ctime (&tihm->release_date));
      fprintf (stdout, " bpm     : %d\n", tihm->bpm);
      fprintf (stdout, " played  : %d\n", tihm->times_played);
      fprintf (stdout, " track   : %d/%d\n", tihm->track, tihm->album_tracks);
      fprintf (stdout, " disk    : %d/%d\n", tihm->disk_num, tihm->disk_total);
      
      for (i = 0 ; i < tihm->num_dohm ; i++) {
	if (tihm->dohms[i].type == IPOD_EQ)
	  fprintf (stdout, " %10s : %i\n", str_type(tihm->dohms[i].type), tihm->dohms[i].data[5]);
	else
	  fprintf (stdout, " %10s : %s\n", str_type(tihm->dohms[i].type), tihm->dohms[i].data);
      }
      
      fprintf (stdout, "\n");
    }
  
  /* free the song list */
  db_song_list_free (&songs);
  
  /* get playlists */
  /* audio playlists */
  playlists = NULL;
  db_playlist_list (&itunesdb, &playlists, 2);

  if (playlists == NULL) {
    fprintf (stdout, "Could not get playlist list\n");
    db_free(&itunesdb);

    exit(1);
  }
  
  /* dump playlist contents */
  for (tmp = db_list_first (playlists) ; tmp ; tmp = db_list_next (tmp)) {
    pyhm = (pyhm_t *)tmp->data;
    /* P(laylist) M(aster) */
    fprintf (stdout, "playlist name: %s(%s) len=%lu\n", pyhm->name, (pyhm->num)?"P":"M", pyhm->name_len);
    
    db_playlist_song_list (&itunesdb, pyhm->num, &list);
    
    for (tmp2 = db_list_first (list) ; tmp2 ; tmp2 = db_list_next (tmp2))
      fprintf (stdout, "%u ", (unsigned int)tmp2->data);
    
    fprintf (stdout, "\n");
    db_playlist_song_list_free(&list);
  }
  
  db_playlist_list_free (&playlists);

  /* video playlists */
  playlists = NULL;  
  db_playlist_list (&itunesdb, &playlists, 3);

  if (playlists == NULL) {
    fprintf (stdout, "Could not get playlist list\n");
    db_free(&itunesdb);

    exit(1);
  }
  
  /* dump playlist contents */
  for (tmp = db_list_first (playlists) ; tmp ; tmp = db_list_next (tmp)) {
    pyhm = (pyhm_t *)tmp->data;
    /* P(laylist) M(aster) */
    fprintf (stdout, "playlist name: %s(%s) len=%lu\n", pyhm->name, (pyhm->num)?"P":"M", pyhm->name_len);
    
    db_playlist_video_list (&itunesdb, pyhm->num, &list);
    
    for (tmp2 = db_list_first (list) ; tmp2 ; tmp2 = db_list_next (tmp2))
      fprintf (stdout, "%u ", (unsigned int)tmp2->data);
    
    fprintf (stdout, "\n");
    db_playlist_song_list_free(&list);
  }
  
  db_playlist_list_free (&playlists);

  db_free(&itunesdb);

  return 0;
}
