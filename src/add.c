#include "itunesdb.h"

#include <stdlib.h>

void usage(void) {
  printf("Usage:\n");
  printf(" db_add <database in> <database out> <song> <mac path>\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  itunesdb_t ipod;
  tihm_t new_entry;
  int ret;

  if (argc != 5)
    usage();

  if ((ret = db_load (&ipod, argv[1], 0x1)) < 0) {
    printf("Could not load database.\n");
    exit(2);
  }

  printf("%i B read from iTunesDB %s.\n", ret, argv[1]);

  if (db_add (&ipod, argv[3], argv[4], strlen(argv[4]), 0, 1) < 0) {
    printf("Song could not be added.\n");
    exit(2);
  }

  if ((ret = db_write (ipod, argv[2])) < 0) {
    printf("Database could not be written to file.\n");
    exit(2);
  }

  db_free(&ipod);
  printf("%i B written to iTunesDB %s.\n", ret, argv[2]);

  return 0;
}
