#include "itunesdb.h"

#include <stdlib.h>

void usage(void) {
  printf("Usage:\n");
  printf(" db_remove <database in> <database out> <tihm number>\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  itunesdb_t ipod;
  int ret;

  if (argc != 4)
    usage();

  if (db_load (&ipod, argv[1]) != 0) {
    printf("Could not load database.\n");
    exit(2);
  }

  if (db_remove (&ipod, atoi(argv[3])) != 0) {
    printf("Song could not be removed.\n");
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