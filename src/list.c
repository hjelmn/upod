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
    printf ("%i\n", dohm_type);
    return "Unknown";
  }
}

int main (int argc, char *argv[]) {
  GList *songs, *tmp;
  GList *playlists;

  tihm_t *tihm;
  pyhm_t *pyhm;
  char buffer[1024];
  int i, ret;

  itunesdb_t itunesdb;
  int fd, result;

  if (argc != 2)
    usage();

  db_set_debug (&itunesdb, 1, stderr);

  if (db_load (&itunesdb, argv[1]) < 0) {
    close (fd);
    exit(1);
  }

  printf ("Checking the sanity of the database...\n");

  ret = db_sanity_check(itunesdb);

  if (ret <= -20) {
    printf ("There is somthing wrong with the loaded database!\n");
    db_free (&itunesdb);
    exit(1);
  } else if (ret < 0) {
    printf ("The database has small problems, continuing anyway. :: %i\n", ret);
  } else
    printf ("The database check out.\n");

  /* get song lists */
  songs = db_song_list (&itunesdb);

  if (songs == NULL)
    printf("Could not get song list\n");

  /* get playlists */
  playlists = db_playlist_list (&itunesdb);

  if (playlists == NULL) {
    printf("Could not get playlist list\n");
    db_free(&itunesdb);
    close (fd);
    exit(1);
  }

  /* dump songlist contents */
  for (tmp = songs ; tmp ; tmp = tmp->next) {
    tihm = tmp->data;

    printf("%04i |\n", tihm->num, buffer);
    
    printf(" encoding: %d\n", tihm->bitrate);
    printf(" type    : %d\n", tihm->type);
    printf(" num dohm: %d\n", tihm->num_dohm);
    printf(" samplert: %d\n", tihm->samplerate);
    printf(" stars   : %d\n", tihm->stars);
    printf(" year    : %d\n", tihm->year);
    printf(" bpm     : %d\n", tihm->bpm);
    printf(" played  : %d\n", tihm->times_played);
    printf(" track   : %d/%d\n", tihm->track, tihm->album_tracks);
    printf(" disk    : %d/%d\n", tihm->disk_num, tihm->disk_total);

    for (i = 0 ; i < tihm->num_dohm ; i++) {
      memset(buffer, 0, 1024);
      unicode_to_char (buffer, tihm->dohms[i].data, tihm->dohms[i].size);
      if (tihm->dohms[i].type == IPOD_EQ)
	printf (" %10s : %i\n", str_type(tihm->dohms[i].type), buffer[5]);
      else
	printf (" %10s : %s\n", str_type(tihm->dohms[i].type), buffer);
    }

    printf("\n");
  }
  
  /* free the song list */
  db_song_list_free (songs);
  songs = NULL;
  
  /* dump playlist contents */
  
  for (tmp = playlists ; tmp ; tmp = tmp->next) {
    int num_ref;
    int *list = NULL;
    
    pyhm = (pyhm_t *)tmp->data;
    /* P(laylist) M(aster) */
    printf ("playlist name: %s(%s) len=%i\n", pyhm->name, (pyhm->num)?"P":"M", pyhm->name_len);
    
    num_ref = db_playlist_list_songs (&itunesdb, pyhm->num, &list);
    
    for (i = 0 ; i < num_ref ; i++)
      printf ("%i ", list[i]);
    
    printf ("\n");
    free(list);
  }
  
  db_playlist_list_free (playlists);
  db_free(&itunesdb);

  close(fd);

  return 0;
}
