#include "itunesdb.h"

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/stat.h>

#include <sys/types.h>
#include <dirent.h>

void usage(void) {
  printf ("Usage:\n");
  printf (" db_dir <database> <dir>\n");
  printf (" db_dir -create <dir> <database>\n");

  exit(1);
}

char *path_unix_mac_root (char *path) {
  char *mac_path = malloc (strlen(path) + 2);
  int i;

  memset (mac_path, 0, strlen(path) + 2);
  mac_path[0] = ':';

  for (i = 0 ; i < strlen (path) ; i++)
    if (path[i] == '/')
      mac_path[i+1] = ':';
    else
      mac_path[i+1] = path[i];

  mac_path[i+1] = '\0';

  return mac_path;
}

int dir_add (itunesdb_t *ipod, char *dir) {
  static int added = 0;
  struct stat statinfo;
  char path_temp[255];
  struct dirent *entry;
  char *tmp;

  DIR *dir_fd;

  if (stat (dir, &statinfo) < 0)
    return added;

  printf ("Adding dir %s\n", dir);

  dir_fd = opendir (dir);

  while (entry = readdir (dir_fd)) {
    memset (path_temp, 0, 255);
    sprintf (path_temp, "%s/%s", dir, entry->d_name);

    stat (path_temp, &statinfo);

    if (S_ISDIR (statinfo.st_mode) && entry->d_name[0] != '.')
      dir_add (ipod, path_temp);
    else if (S_ISREG (statinfo.st_mode) && entry->d_name[0] != '.') {
      added++;
      //      fprintf (stderr, "Adding %s.\n", path_temp);
      db_add (ipod, path_temp, tmp = path_unix_mac_root(path_temp), strlen(tmp), rand()%6);
    }
  }

  closedir (dir_fd);

  return added;
}

int main(int argc, char *argv[]) {
  itunesdb_t itunesdb;
  int c = 0, i;
  int ret;

  if (argc < 3)
    usage();

  memset (&itunesdb, 0, sizeof (itunesdb_t));

  if (strcmp (argv[1], "-create") == 0) {
    if (argc < 4) usage();
    printf ("Creating a database... ");
    if ((ret = db_create(&itunesdb, "iPod", 4)) < 0) {
      printf("Could not create database.\n");
      exit(2);
    }
    printf ("done.\n");
    c = 1;
  } else {
    if (argc < 3) usage();
    printf ("Loading a database.\n");
    if ((ret = db_load (&itunesdb, argv[1])) < 0) {
      printf("Could not open database.\n");
      exit(2);
    }

    printf("%i B read from iTunesDB %s.\n", ret, argv[1]);
  }

  for (i = 2 ; i < (argc - 1) ; i++)
    dir_add (&itunesdb, argv[2]);

  if ((ret = db_sanity_check (itunesdb)) < 0) {
    printf ("There is an error in the tree: %i\n", ret);
  }

  if (c == 0) {
    if ((ret = db_write (itunesdb, argv[1])) < 0) {
      printf("Database could not be written to file.\n");
      exit(2);
    }
  } else {
    if ((ret = db_write (itunesdb, argv[3])) < 0) {
      printf("Database could not be written to file.\n");
      exit(2);
    }
  }


  db_free(&itunesdb);
  printf("%i B written to iTunesDB %s.\n", ret, argv[(c?(argc-1):2)]);
    
  return 0;
}
