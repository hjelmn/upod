#include "itunesdb.h"

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <libgen.h>

#include <sys/stat.h>

#include <sys/types.h>
#include <dirent.h>

void usage(void) {
  printf("Usage:\n");
  printf(" itdbadd_dir <database> <files ...>\n");
  printf(" itdbadd_dir -create <database> <files ...>\n");

  exit(1);
}

char *path_unix_mac_root (char *path) {
  char *mac_path;
  int i;

  if (path[0] == '/')
    path++;

  mac_path = calloc (strlen(path) + 1, 1);
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
  int added = 0;
  struct stat statinfo;
  char path_temp[255];
  struct dirent *entry;
  char *tmp;

  DIR *dir_fd;
  
  tmp = basename (dir);

  /* no such file or dot file */
  if (stat (dir, &statinfo) < 0 || tmp[0] == '.')
    return 0;

  /* regular files get inserted into the database */
  if (S_ISREG(statinfo.st_mode)) {
    if (db_add (ipod, dir, tmp = path_unix_mac_root(dir), strlen(tmp), rand()%6) < 0)
      return 0;
    else
      return 1;
  }

  printf ("Adding dir %s\n", dir);

  dir_fd = opendir (dir);

  while (entry = readdir (dir_fd)) {
    memset (path_temp, 0, 255);

    if (dir[strlen(dir) - 1] != '/')
      snprintf (path_temp, 255, "%s/%s", dir, entry->d_name);
    else
      snprintf (path_temp, 255, "%s%s", dir, entry->d_name);

    added += dir_add (ipod, path_temp);
  }

  closedir (dir_fd);

  return added;
}

int main(int argc, char *argv[]) {
  itunesdb_t itunesdb;
  int c = 0;
  int ret;
  int dbi, i;

  if (argc < 3) usage ();

  memset (&itunesdb, 0, sizeof  (itunesdb_t));
  db_set_debug (&itunesdb, 0, stderr);

  if (strcmp (argv[1], "-create") == 0) {
    if (argc < 4) usage();
    printf ("Creating a database... ");
    if ((ret = db_create(&itunesdb, "iPod", 4)) < 0) {
      printf("Could not create database.\n");
      exit(2);
    }
    printf ("done.\n");
    c = 1;

    dbi = 2;
  } else {
    if (argc < 3) usage();
    dbi = 1;
    printf ("Loading a database.\n");
    if ((ret = db_load (&itunesdb, argv[dbi])) < 0) {
      printf("Could not open database.\n");
      exit(2);
    }

    printf("%i B read from iTunesDB %s.\n", ret, argv[1]);
  }

  for (i = dbi + 1 ; i < argc ; i++)
    dir_add (&itunesdb, argv[i]);

  if ((ret = db_sanity_check (itunesdb)) < 0) {
    printf ("There is an error in the tree: %i\n", ret);
  }

  if ((ret = db_write (itunesdb, argv[dbi])) < 0) {
    printf("Database could not be written to file.\n");
    exit(2);
  }


  db_free(&itunesdb);
  printf("%i B written to iTunesDB %s.\n", ret, argv[dbi]);

  return 0;
}
