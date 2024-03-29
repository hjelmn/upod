/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 pl.c
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
#include <string.h>

#include "itunesdb.h"

#include <stdlib.h>

void usage(void) {
  printf ("Usage:\n");
  printf ("  db_pl <db> add <int> <tihm ids>\n");
  printf ("  db_pl <db> clear <int>\n");
  printf ("  db_pl <db> create <name>\n");
  printf ("  db_pl <db> list\n");
  printf ("  db_pl <db> remove <int> <tihm ids>\n");
  printf ("  db_pl <db> rename <int> <name>\n");
  printf ("  db_pl <db> songs <int>\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  ipoddb_t itunesdb;
  db_list_t *list, *reflist, *tmp;
  int i;
  int ret;
  int qlist = 0;
  int cr = 0;
  int rn = 0;
  int cl = 0;
  int add = 0;
  int remove = 0;

  if (argc < 3)
    usage();

  db_set_debug (&itunesdb, 5, stderr);

  if (strstr (argv[2], "list") != NULL)
    qlist = 1;
  else if (strstr (argv[2], "clear") != NULL)
    cl = 1;
  else if (strstr (argv[2], "add") != NULL)
    add = 1;
  else if (strstr (argv[2], "remove") != NULL)
    remove = 1;
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
    db_playlist_list (&itunesdb, &list, 2);
    
    for (tmp = db_list_first (list) ; tmp ; tmp = db_list_next (tmp))
      printf ("List %i: %s\n", ((pyhm_t *)tmp->data)->num,
	      ((pyhm_t *)tmp->data)->name);
    db_playlist_list_free(&list);
  } else if (cr) {
    if (db_playlist_create (&itunesdb, argv[3], 2) > 0) {
      db_write (itunesdb, argv[1]);
    }

  } else if (rn) {
    if (db_playlist_rename (&itunesdb, strtol(argv[3], NULL, 10), 2, argv[4]) == 0) {
      db_write (itunesdb, argv[1]);
    }
  } else if (cl) {
    if (db_playlist_clear (&itunesdb, strtol(argv[3], NULL, 10), 2) == 0) {
      db_write (itunesdb, argv[1]);
    }
  } else if (add) {
    for (i = 4 ; i < argc ; i++)
      db_playlist_tihm_add (&itunesdb, strtol(argv[3], NULL, 10), 2, strtol(argv[i], NULL, 10));

    db_write (itunesdb, argv[1]);
  } else if (remove) {
    for (i = 4 ; i < argc ; i++)
      db_playlist_tihm_remove (&itunesdb, strtol(argv[3], NULL, 10), 2, strtol(argv[i], NULL, 10));

    db_write (itunesdb, argv[1]);
  } else {
    if ((ret = db_playlist_song_list (&itunesdb, atoi(argv[3]),
				      &reflist)) > 0) {
      for (tmp = db_list_first (reflist) ; tmp ; tmp = db_list_next (tmp))
	printf("Reference: %08x\n", (unsigned int)(tmp->data));

      db_playlist_song_list_free (&reflist);
    }
  }

  db_free(&itunesdb);
  return 0;
}
