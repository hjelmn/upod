#include "itunesdb.h"

#include <stdlib.h>
#include <stdio.h>

void usage (void) {
  printf("Usgae:\n");
  printf(" db_list <database>\n");

  exit(1);
}

char *str_type(int dohm_type) {
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
  default:
    return "Unknown";
  }
}

int main (int argc, char *argv[]) {
  GList *songs, *tmp, *tmp2, *list;
  GList *playlists;

  tihm_t *tihm;
  pyhm_t *pyhm;
  char *buffer;
  size_t buffer_len;
  int i, ret;
  
  itunesdb_t itunesdb;
  int fd, result;

  if (argc != 2)
    usage();

  db_set_debug (&itunesdb, 5, stderr);

  if (db_load (&itunesdb, argv[1], 0x0) < 0) {
    close (fd);
    exit(1);
  }

  fprintf (stdout, "Checking the sanity of the database...\n");

  ret = db_sanity_check(itunesdb);

  if (ret <= -20) {
    fprintf (stdout, "There is somthing wrong with the loaded database!\n");
    db_free (&itunesdb);
    exit(1);
  } else if (ret < 0) {
    fprintf (stdout, "The database has small problems, continuing anyway. :: %i\n", ret);
  } else
    fprintf (stdout, "The database checks out.\n");

  /* get song lists */
  db_song_list (&itunesdb, &songs);

  if (songs == NULL)
    fprintf (stdout, "Could not get song list\n");
  else
    /* dump songlist contents */
    for (tmp = g_list_first (songs) ; tmp ; tmp = g_list_next (tmp)) {
      tihm = tmp->data;
      
      fprintf (stdout, "%04i |\n", tihm->num, buffer);
      
      fprintf (stdout, " encoding: %d\n", tihm->bitrate);
      fprintf (stdout, " length  : %03.2f\n", (double)tihm->time/60000.0);
      fprintf (stdout, " type    : %d\n", tihm->type);
      fprintf (stdout, " num dohm: %d\n", tihm->num_dohm);
      fprintf (stdout, " samplert: %d\n", tihm->samplerate);
      fprintf (stdout, " stars   : %d\n", tihm->stars);
      fprintf (stdout, " year    : %d\n", tihm->year);
      fprintf (stdout, " bpm     : %d\n", tihm->bpm);
      fprintf (stdout, " played  : %d\n", tihm->times_played);
      fprintf (stdout, " track   : %d/%d\n", tihm->track, tihm->album_tracks);
      fprintf (stdout, " disk    : %d/%d\n", tihm->disk_num, tihm->disk_total);
      
      for (i = 0 ; i < tihm->num_dohm ; i++) {
	unicode_to_utf8 (&buffer, &buffer_len, tihm->dohms[i].data, tihm->dohms[i].size);
	if (tihm->dohms[i].type == IPOD_EQ)
	  fprintf (stdout, " %10s : %i\n", str_type(tihm->dohms[i].type), buffer[5]);
	else
	  fprintf (stdout, " %10s : %s\n", str_type(tihm->dohms[i].type), buffer);
	
	if (buffer)
	  free (buffer);
      }
      
      fprintf (stdout, "\n");
    }
  
  /* free the song list */
  db_song_list_free (&songs);
  
  /* get playlists */
  db_playlist_list (&itunesdb, &playlists);

  if (playlists == NULL) {
    fprintf (stdout, "Could not get playlist list\n");
    db_free(&itunesdb);
    close (fd);
    exit(1);
  }
  
  /* dump playlist contents */
  for (tmp = g_list_first (playlists) ; tmp ; tmp = g_list_next (tmp)) {
    pyhm = (pyhm_t *)tmp->data;
    /* P(laylist) M(aster) */
    fprintf (stdout, "playlist name: %s(%s) len=%i\n", pyhm->name, (pyhm->num)?"P":"M", pyhm->name_len);
    
    db_playlist_song_list (&itunesdb, pyhm->num, &list);
    
    for (tmp2 = g_list_first (list) ; tmp2 ; tmp2 = g_list_next (tmp2))
      fprintf (stdout, "%u ", (unsigned int)tmp2->data);
    
    fprintf (stdout, "\n");
    db_playlist_song_list_free(&list);
  }
  
  db_playlist_list_free (&playlists);
  db_free(&itunesdb);

  close(fd);

  return 0;
}
