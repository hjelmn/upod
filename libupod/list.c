#include "upodi.h"

#include <stdlib.h>
#include <stdio.h>

void usage (void) {
  printf("Usgae:\n");
  printf(" db_list <database>\n");

  exit(1);
}

char *str_type(int file_type) {
  switch (file_type) {
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
  default:
    return "Unknown";
  }
}

int main (int argc, char *argv[]) {
  ipod_t ipod;
  GList *songs, *tmp;
  tihm_t *tihm;
  char buffer[1024];
  int i;

  if (argc != 2)
    usage();

  if (db_load (&ipod, argv[1]) < 0) {
    printf("Could not load database\n");

    exit(2);
  }

  songs = db_song_list (ipod);

  if (songs == NULL) {
    printf("Could not get song list\n");
    db_free(&ipod);
    exit(1);
  }

  for (tmp = songs->next ; tmp ; tmp = tmp->next) {
    tihm = tmp->data;

    printf("%04i |\n", tihm->num, buffer);
    
    printf(" encoding: %08x\n", tihm->encoding);
    printf(" type    : %d\n", tihm->type);
    printf(" num dohm: %d\n", tihm->num_dohm);
    printf(" samplert: %d\n", tihm->samplerate);

    for (i = 0 ; i < tihm->num_dohm ; i++) {
      memset(buffer, 0, 1024);
      unicode_to_char (buffer, tihm->dohms[i].data, tihm->dohms[i].size);
      printf(" %10s : %s\n", str_type(tihm->dohms[i].type), buffer);
    }

    printf("\n");
  }

  db_free(&ipod);
  db_song_list_free (songs);

  return 0;
}
