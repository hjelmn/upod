#include "itunesdb.h"

#include <stdlib.h>

void usage(void) {
  printf("Usage:\n");
  printf(" db_remove <database in> <database out> <tihm number>\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  itunesdb_t itunesdb;
  int ret;

  if (argc != 4)
    usage();

  db_set_debug (&itunesdb, 2, stderr);

  if (db_load (&itunesdb, argv[1], 0x1) < 0) {
    printf("Could not load database.\n");
    exit(2);
  }

  if (db_remove (&itunesdb, atoi(argv[3])) != 0) {
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
