#include "upod.h"

#include <stdlib.h>

void usage(void) {
  printf("Usage:\n");
  printf(" db_add <database in> <database out> <song> <mac path>\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  ipod_t ipod;
  int ret;

  if (argc != 5)
    usage();

  if (db_load (&ipod, argv[1]) != 0) {
    printf("Could not load database.\n");
    exit(2);
  }

  if (db_add (&ipod, argv[3], argv[4]) != 0) {
    printf("Song could not be added.\n");
    exit(2);
  }

  if ((ret = db_write (ipod, argv[2])) < 0) {
    printf("Database could not be written to file.\n");
    exit(2);
  }

  printf("%i B written to iTunesDB %s.\n", ret, argv[2]);

  return 0;
}
