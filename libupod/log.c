/**
 *   (c) 2004-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 log.c
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the Lesser GNU Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the Lesser GNU Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "itunesdbi.h"

int db_set_debug (ipoddb_t *itunesdb, int level, FILE *out) {
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

void db_log (ipoddb_t *itunesdb, int error, char *format, ...) {
  if ( (itunesdb->log_level > 0) && (itunesdb->log != NULL) ) {
    va_list arg;

    va_start (arg, format);

    if (error == 0) {
      vfprintf (itunesdb->log, format, arg);
    } else {
      fprintf(itunesdb->log, "Error %i| ", error);
      vfprintf (itunesdb->log, format, arg);
    }

    fflush (itunesdb->log);

    va_end (arg);
  }
}
