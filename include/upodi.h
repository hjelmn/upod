
typedef struct _ipod {
  /* mount information */
  char path[255];
  char is_mounted;

  int db_fd;

  int db_size_current;

  int debug;

  int first_entry;
  int last_entry;
  
  int num_playlists;
  
  /* this pointer could possibly be huge (the size of the database) */
  char *itunesdb;

  char *insertion_point;
} ipod_t;
