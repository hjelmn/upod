#include <string.h>

#include "itunesdb.h"

#include <stdlib.h>

void usage(void) {
  printf("Usage:\n");
  printf("  db_pl <db> list\n");
  printf("  db_pl <db> songs <int>\n");
  printf("  db_pl <db> create <name>\n");
  printf("  db_pl <db> rename <int> <name>\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  itunesdb_t itunesdb;
  GList *list, *reflist, *tmp;
  int ret;
  int qlist = 0;
  int cr = 0;
  int rn = 0;

  if (argc < 3 || argc > 5)
    usage();

  db_set_debug (&itunesdb, 5, stderr);

  if (strstr (argv[2], "list") != NULL)
    qlist = 1;
  else if (strstr (argv[2], "songs") != NULL)
    qlist = 0;
  else if (strstr (argv[2], "create") != NULL && argc == 4)
    cr = 1;
  else if (strstr (argv[2], "rename") != NULL && argc == 5)
    rn = 1;
  else
    usage();

  if ((ret = db_load (&itunesdb, argv[1], 0x0)) < 0) {
    printf("Could not load database.\n");
    exit(2);
  }
  printf("%i B read from iTunesDB %s.\n", ret, argv[1]);

  if (qlist) {
    db_playlist_list (&itunesdb, &list);
    
    for (tmp = g_list_first (list) ; tmp ; tmp = g_list_next (tmp))
      printf ("List %i: %s\n", ((pyhm_t *)tmp->data)->num,
	      ((pyhm_t *)tmp->data)->name);
    db_playlist_list_free(&list);
  } else if (cr) {
    if (db_playlist_create (&itunesdb, argv[3], strlen(argv[3])) > 0) {
      db_write (itunesdb, argv[1]);
    }

  } else if (rn) {
    if (db_playlist_rename (&itunesdb, strtol(argv[3], NULL, 10), argv[4],
			    strlen(argv[4])) == 0) {
      db_write (itunesdb, argv[1]);
    }
  } else {
    int i;
    if ((ret = db_playlist_song_list (&itunesdb, atoi(argv[3]),
				      &reflist)) > 0) {
      for (tmp = g_list_first (reflist) ; tmp ; tmp = g_list_next (tmp))
	printf("Reference: %08x\n", (unsigned int)(tmp->data));

      db_playlist_song_list_free (&reflist);
    }
  }

  db_free(&itunesdb);
  return 0;
}
