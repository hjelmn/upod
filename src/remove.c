/**
 *   (c) 2002-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 remove.c
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

void usage(void) {
  printf("Usage:\n");
  printf(" db_remove <database in> <database out> <tihm number>\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  ipoddb_t itunesdb;
  int ret;

  if (argc != 4)
    usage();

  db_set_debug (&itunesdb, 2, stderr);

  if (db_load (&itunesdb, argv[1], 0x1) < 0) {
    printf("Could not load database.\n");
    exit(2);
  }

  if (db_song_remove (&itunesdb, atoi(argv[3])) != 0) {
    printf("Song could not be removed.\n");
    exit(2);
  }

  if ((ret = db_write (itunesdb, argv[2])) < 0) {
    printf("Database could not be written to file.\n");
    exit(2);
  }

  db_free(&itunesdb);
  printf("%i B written to iTunesDB %s.\n", ret, argv[2]);

  return 0;
}
