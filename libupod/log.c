#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "itunesdbi.h"

int db_set_debug (itunesdb_t *itunesdb, int level, FILE *out) {
  int slevel;

  if (out == NULL)
    return -1;
  
  if (level < 0)
    slevel = 0;
  else if (level > 5)
    slevel = 5;
  else
    slevel = level;

  fprintf (out, "Setting log level to %i\n", slevel);

  itunesdb->log_level = slevel;
  itunesdb->log       = out;

  return slevel;
}

void db_log (itunesdb_t *itunesdb, int error, char *format, ...) {
  if ( (itunesdb->log_level > 0) && (itunesdb->log != NULL) ) {
    va_list arg;

    va_start (arg, format);

    if (error == 0) {
      vfprintf (itunesdb->log, format, arg);
    } else {
      fprintf(itunesdb->log, "Error %i| ", error);
      vfprintf (itunesdb->log, format, arg);
    }

    va_end (arg);
  }
}
