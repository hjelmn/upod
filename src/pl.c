#include <string.h>

#include "itunesdb.h"

#include <stdlib.h>

void usage(void) {
  printf("Usage:\n");
  printf("  db_pl <db> list\n");
  printf("  db_pl <db> songs <int>\n");
  printf("  db_pl <db> create <name>\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  itunesdb_t ipod;
  GList *list;
  int *reflist;
  int ret;
  int qlist = 0;
  int cr = 0;

  if (argc < 3 || argc > 4)
    usage();

  if (strstr (argv[2], "list") != NULL)
    qlist = 1;
  else if (strstr (argv[2], "songs") != NULL)
    qlist = 0;
  else if (strstr (argv[2], "create") != NULL && argc == 4)
    cr = 1;
  else
    usage();

  if ((ret = db_load (&ipod, argv[1])) < 0) {
    printf("Could not load database.\n");
    exit(2);
  }
  printf("%i B read from iTunesDB %s.\n", ret, argv[1]);

  if (qlist) {
    GList *tmp;
    list = db_playlist_list (&ipod);
    
    for (tmp = list ; tmp ; tmp = tmp->next)
      printf ("List %i: %s\n", ((pyhm_t *)tmp->data)->num, ((pyhm_t *)tmp->data)->name);
    db_playlist_list_free(list);
  } else if (cr) {
    if (db_playlist_create (&ipod, argv[3], strlen(argv[3])) > 0) {
      db_write (ipod, argv[1]);
    }

  } else {
    int i;
    if ((ret = db_playlist_list_songs (&ipod, atoi(argv[3]), &reflist)) > 0) {
      for (i = 0 ; i < ret ; i++)
	printf("Reference: %08x\n", reflist[i]);
    }
  }

  db_free(&ipod);
  return 0;
}
