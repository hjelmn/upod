/**
 *   (c) 2004-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v1.2.0 ipod_update.c
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


#if defined (HAVE_CONFIG_H)
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <glib.h>

#include "itunesdb.h"

#define PACKAGE "upod"
#define VERSION "2.1"

void usage (void);
void version (void);

static u_int8_t *path_unix_mac_root (char *path) {
  char *mac_path;
  int i;

  if (*path == '/') path++;

  mac_path = calloc (strlen(path) + 2, 1);
  mac_path[0] = ':';

  for (i = 0 ; i < strlen (path) ; i++)
    if (path[i] == '/')
      mac_path[i+1] = ':';
    else
      mac_path[i+1] = path[i];

  mac_path[i+1] = '\0';

  return mac_path;
}


static u_int8_t *path_mac_unix (char *mac_path) {
  char *path;
  int i;

  if (*mac_path == ':') mac_path++;

  path = calloc (strlen(mac_path) + 1, 1);

  for (i = 0 ; i < strlen (mac_path) ; i++)
    if (mac_path[i] == ':')
      path[i] = '/';
    else
      path[i] = mac_path[i];

  path[i] = '\0';

  return path;
}

int glist_cmp (gpointer data1, gpointer data2) {
  return (strcasecmp ((char *)data1, (char *)data2) == 0) ? 1 : 0;
}

void print_parsed (void) {
  static int parsed = 0;

  parsed++;

  printf ("Parsed %05i files.", parsed);
  printf ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
  fflush (stdout);
}

int parse_dir (char *path, ipoddb_t *itunesdb, ipoddb_t *artworkdb, GList *hidden,
	       int playlist) {
  char scratch[1024];

  DIR *dirp;
  FILE *fh;
  struct dirent *dirent;
  struct stat statinfo;
  int tihm_num;
  int songs_added = 0;

  dirp = opendir (path);

  sprintf (scratch, "%s/.hidden_songs", path);
  if (stat (scratch, &statinfo) == 0) {
    fh = fopen (scratch, "r");
    if (fh == NULL) {
      perror (scratch);

      return -errno;
    }

    while (fscanf (fh, "%s", scratch) > 0) {
      char *tmp = strdup (scratch);

      hidden = g_list_append (hidden, tmp);
    }
  }

  while ((dirent = readdir (dirp)) != NULL) {
    char *mac_path;

    if (dirent->d_name[0] == '.')
      continue;

    sprintf (scratch, "%s/%s", path, dirent->d_name);

    if (stat (scratch, &statinfo) < 0)
      continue;

    if (!S_ISDIR (statinfo.st_mode)) {
      GList *tmp;

      print_parsed ();
      mac_path = path_unix_mac_root (scratch);
      if ((tmp = g_list_find_custom (hidden, scratch,
				     (GCompareFunc)glist_cmp)) != NULL) {
	tihm_num = db_song_add (itunesdb, artworkdb, scratch, mac_path, 0, 0);

	if (tihm_num < 0 && ((tihm_num = db_lookup (itunesdb, IPOD_PATH, mac_path)) >= 0)) {
	  db_song_hide (itunesdb, tihm_num);
	  tihm_num = -1;
	}

	free (tmp->data);
	hidden = g_list_delete_link (hidden, tmp);
      } else {
	tihm_num = db_song_add (itunesdb, artworkdb, scratch, mac_path, 0, 1);

	if (tihm_num < 0 && ((tihm_num = db_lookup (itunesdb, IPOD_PATH, mac_path)) >= 0)) {
	  db_song_unhide (itunesdb, tihm_num);
	  tihm_num = -1;
	}
      }
      if (tihm_num >= 0) {
	if (playlist != -1)
	  db_playlist_tihm_add (itunesdb, playlist, tihm_num);
	
	songs_added++;
      }

      free (mac_path);
    } else
      parse_dir (scratch, itunesdb, artworkdb, hidden, playlist);
  }

  closedir (dirp);

  if (songs_added)
    printf ("Added %s: %i songs\n", path, songs_added);

  return 0;
}

int parse_playlists (char *path, ipoddb_t *itunesdb, ipoddb_t *artworkdb) {
  char scratch[1024];

  DIR *dirp;
  struct dirent *dirent;
  struct stat statinfo;

  dirp = opendir (path);
  if (dirp == NULL) {
    perror (path);
    return -errno;
  }

  while ((dirent = readdir (dirp)) != NULL) {
    int playlist = -1;

    if (dirent->d_name[0] == '.')
      continue;

    sprintf (scratch, "%s/%s", path, dirent->d_name);

    if (stat (scratch, &statinfo) < 0)
      continue;

    if (S_ISDIR (statinfo.st_mode) && 
	strcasecmp(dirent->d_name, "main_playlist")) {
      playlist = db_lookup_playlist (itunesdb, dirent->d_name);

      if (playlist < 0) {
	printf ("Creating playlist: %s\n", dirent->d_name);
	playlist = db_playlist_create (itunesdb, dirent->d_name);
      }
    }

    parse_dir (scratch, itunesdb, artworkdb, NULL, playlist);
  }
  
  closedir (dirp);

  return 0;
}

int write_itdatabase (ipoddb_t *itunesdb) {
  struct stat statinfo;
  int mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

  if (stat ("iPod_Control", &statinfo) < 0)
    mkdir ("iPod_Control", mode);
  else if (!S_ISDIR (statinfo.st_mode)) {
    fprintf (stderr, "iPod_Control is not a directory!\n");
    return -1;
  }

  if (stat ("iPod_Control/iTunes", &statinfo) < 0)
    mkdir ("iPod_Control/iTunes", mode);
  else if (!S_ISDIR (statinfo.st_mode)) {
    fprintf (stderr, "iPod_Control/iTunesDB is not a directory!\n");
    return -1;
  }

  return db_write (*itunesdb, ITUNESDB);
}

int write_awdatabase (ipoddb_t *artworkdb) {
  struct stat statinfo;
  int mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

  if (stat ("iPod_Control", &statinfo) < 0)
    mkdir ("iPod_Control", mode);
  else if (!S_ISDIR (statinfo.st_mode)) {
    fprintf (stderr, "iPod_Control is not a directory!\n");
    return -1;
  }

  if (stat ("iPod_Control/Artwork", &statinfo) < 0)
    mkdir ("iPod_Control/Artwork", mode);
  else if (!S_ISDIR (statinfo.st_mode)) {
    fprintf (stderr, "iPod_Control/Artwork is not a directory!\n");
    return -1;
  }

  return db_write (*artworkdb, ARTWORKDB);
}

/* remove files that no longer exist from the database */
int cleanup_database (ipoddb_t *itunesdb) {
  GList *tmp, *song_list = NULL;
  tihm_t *tihm;
  char *unix_path;
  int i;

  struct stat statinfo;

  db_song_list (itunesdb, &song_list);

  if (song_list == NULL)
    return -1;

  for (tmp = g_list_first (song_list) ; tmp ; tmp = g_list_next (tmp)) {
    tihm = (tihm_t *)tmp->data;

    for (i = 0 ; i < tihm->num_dohm ; i++)
      if (tihm->dohms[i].type == IPOD_PATH)
	break;
    if (i == tihm->num_dohm)
      continue;

    unix_path = path_mac_unix (tihm->dohms[i].data);

    if (stat (unix_path, &statinfo) < 0) {
      printf ("%s(%s): no longer exists. Removing from the iTunesDB\n", unix_path, tihm->dohms[i].data);
      db_song_remove (itunesdb, tihm->num);
    }

    free (unix_path);
  }

  db_song_list_free (&song_list);

  return 0;
}

int main (int argc, char *argv[]) {
  ipoddb_t itunesdb, artworkdb;
  int option_index;
  int debug_level = 0;
  int create = 0;
  char *ipod_name = NULL;
  int flags = FLAG_UNICODE_HACK;
  int noartwork = 0;
  char c;

  int ret;

  struct option long_options[] = {
    {"noartwork",     0, 0, 'n'},
    {"help",          0, 0, '?'},
    {"create",        1, 0, 'c'},
    {"debug",         0, 0, 'd'},
    {"itunes_compat", 0, 0, 't'},
    {"version",       0, 0, 'v'},
  };

  while ((c = getopt_long (argc, argv, "?c:dtv", long_options,
			   &option_index)) != -1) {
    switch (c) {
    case '?':
      usage ();
      break;
    case 'c':
      create = 1;
      ipod_name = optarg;
      break;
    case 'd':
      debug_level++;
      break;
    case 't':
      flags = flags & ~(FLAG_UNICODE_HACK);
      break;
    case 'n':
      noartwork = 1;
      break;
    case 'v':
      version ();
      break;
    default:
      printf("Unrecognized option -%c.\n\n", c);
      usage();
    }
  }

  memset (&itunesdb, 0, sizeof (ipoddb_t));

  db_set_debug (&itunesdb, debug_level, stderr);

  if (create == 0) {
    if ((ret = db_load (&itunesdb, ITUNESDB, flags)) < 0) {
      fprintf (stderr, "Could not open iTunesDB: %s\n", ITUNESDB);

      exit (1);
    }

    if (noartwork == 0) {
      if ((ret = db_load (&artworkdb, ARTWORKDB, flags)) < 0) {
	if ((ret = db_photo_create (&artworkdb)) < 0) {
	  fprintf (stderr, "Error creating ArtworkDB\n");
	  
	  exit (1);
	}
      }
    }
  } else {
    if ((ret = db_create (&itunesdb, ipod_name, flags)) < 0) {
      fprintf (stderr, "Error creating iTunesDB\n");
      
      exit (1);
    }

    if (noartwork == 0) {
      if ((ret = db_photo_create (&artworkdb)) < 0) {
	fprintf (stderr, "Error creating ArtworkDB\n");
	
	exit (1);
      }
    }
  }

  itunesdb.path  = strdup(ITUNESDB);
  artworkdb.path = strdup(ARTWORKDB);

  cleanup_database (&itunesdb);

  if (noartwork == 1)
    parse_playlists ("Music", &itunesdb, NULL);
  else
    parse_playlists ("Music", &itunesdb, &artworkdb);

  ret = write_itdatabase (&itunesdb);
  printf ("%i B written to the iTunesDB: %s\n", ret, ITUNESDB);
  if (noartwork == 0) {
    ret = write_awdatabase (&artworkdb);
    printf ("%i B written to the ArtworkDB: %s\n", ret, ARTWORKDB);
  }

  db_free (&itunesdb);
  db_free (&artworkdb);

  return 0;
}

void usage (void) {
  printf ("Usage: ipod_update [OPTIONS]\n\n");

  printf ("Update the database in iPod_Control/iTunes/iTunesDB\n");
  printf ("with music from the Music folder.\n\n");

  printf ("  -n, --noartwork      supress modifying the ArtworkDB\n");
  printf ("  -c, --create=<name>  create new database\n");
  printf ("  -d, --debug          increase debuging verbosity\n");
  printf ("  -t, --itunes_compat  turn on itunes compatability for files\n"
	  "                       with non-ASCII characters in their name\n");
  printf ("  -?, --help           print this screen\n");
  printf ("  -v, --version        print version\n");

  exit (0);
}

void version(void){
  printf("%s %s\n", PACKAGE, VERSION);
  printf("Copyright (C) 2004 Nathan Hjelm\n\n");
  
  printf("%s comes with NO WARRANTY.\n", PACKAGE);
  printf("You may redistribute copies of %s under the terms\n", PACKAGE);
  printf("of the GNU Lesser Public License.\n");
  printf("For more information about these issues\n");
  printf("see the file named COPYING in the %s distribution.\n", PACKAGE);
  
  exit (0);
}
