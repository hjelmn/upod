#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <libgen.h>

#include <sys/stat.h>

#include <sys/types.h>
#include <dirent.h>

#include "itunesdb.h"

void usage(void) {
  printf("Usage:\n");
  printf(" itdbadddir -create <database> <file ...>\n");
  printf(" itdbadddir <database> <file ...>\n");

  exit(1);
}

static void path_unix_mac_root (char *path, char mac_path[255]) {
  int i;

  memset (mac_path, 0, 255);

  if (*path == '/')
    path++;

  mac_path[0] = ':';

  for (i = 0 ; i < strlen (path) ; i++)
    if (path[i] == '/')
      mac_path[i+1] = ':';
    else
      mac_path[i+1] = path[i];
}

int dir_add (itunesdb_t *ipod, char *dir) {
  int added = 0;
  struct stat statinfo;
  char *tmp, *tdir;
  char path_temp[255];

  int ret;

  tdir = strdup (dir);
  tmp = basename (tdir);

  if (tmp[0] == '.' || stat (dir, &statinfo) < 0) {
    /* no such file or dot file */
    added = 0;
  } else if (S_ISREG (statinfo.st_mode) || S_ISLNK(statinfo.st_mode)) {
    /* regular files get inserted into the database */
    path_unix_mac_root (dir, path_temp);
    ret = db_add (ipod, dir, path_temp, strlen(path_temp), 0, 1);

    added = (ret < 0) ? 0 : 1;
  } else if (S_ISDIR (statinfo.st_mode)) {
    DIR *dir_fd;
    struct dirent *entry;

    printf ("Adding dir %s\n", dir);
    dir_fd = opendir (dir);
    
    while ((entry = readdir (dir_fd)) != NULL) {
      memset (path_temp, 0, 255);
      sprintf (path_temp, "%s/%s", dir, entry->d_name);
      
      added += dir_add (ipod, path_temp);
    }

    closedir (dir_fd);
  }

  free (tdir);

  return added;
}

int main(int argc, char *argv[]) {
  itunesdb_t itunesdb;
  int c = 0;
  int ret;
  int db, i;
  
  memset (&itunesdb, 0, sizeof  (itunesdb_t));
  db_set_debug (&itunesdb, 0, stderr);

  if (strcmp (argv[1], "-create") == 0) {
    if (argc < 4) usage();
    printf ("Creating a database... ");

    if ((ret = db_create(&itunesdb, "iPod", 4, 0x1)) < 0) {
      printf("Could not create database.\n");
      exit(2);
    }
    printf ("done.\n");
    c = 1;
    db = 2;
  } else {
    if (argc < 3) usage();
    printf ("Loading a database.\n");
    if ((ret = db_load (&itunesdb, argv[1], 0x1)) < 0) {
      printf("Could not open database.\n");
      exit(2);
    }

    printf( "%i B read from iTunesDB %s.\n", ret, argv[1]);
    db = 1;
  }

  for (i = (db + 1) ; i < argc ; i++)
    dir_add (&itunesdb, argv[i]);

  if ((ret = db_sanity_check (itunesdb)) < 0) {
    printf ( "There is an error in the tree: %i\n", ret);
  }

  if ((ret = db_write (itunesdb, argv[db])) < 0) {
    printf ("Database could not be written to file.\n");
    exit(2);
  }

  db_free (&itunesdb);
  printf ( "%i B written to the iTunes database: %s.\n", ret, argv[db]);

  return 0;
}
