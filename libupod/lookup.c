#include "upod.h"

#include <stdlib.h>

void usage(void) {
  printf("Usage:\n");
  printf(" db_lookup <database> <int entry_type> <string data>.\n");

  exit(1);
}

int main(int argc, char *argv[]) {
  ipod_t ipod;
  int ret;

  if (argc != 4)
    usage();

  if ((ret = db_load (&ipod, argv[1])) < 0) {
    printf("Could not load database.\n");
    exit(2);
  }

  printf("%i B read from iTunesDB %s.\n", ret, argv[1]);

  if ((ret = db_lookup (ipod, atoi(argv[2]), argv[3], strlen(argv[3]))) < 0)
    printf("No matching entry found.\n");
  else
    printf("Entry %d matches.\n", ret);

  db_free(&ipod);

  return 0;
}
