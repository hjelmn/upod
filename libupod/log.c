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

int db_set_debug (ipoddb_t *db, int level, FILE *out) {
  int slevel;

  if (out == NULL)
    return -1;
  
  if (level < 0)
    slevel = 0;
  else if (level > 5)
    slevel = 5;
  else
    slevel = level;
  
  if (slevel > 0)
    fprintf (out, "Setting log level to %i\n", slevel);

  db->log_level = slevel;
  db->log       = out;

  return slevel;
}

void db_log (ipoddb_t *db, int error, char *format, ...) {
  FILE *log_out;
  int log_level = 5;

  if (db != NULL) {
    log_out = db->log;
    log_level = db->log_level;
  } else
    log_out = stderr;

  if ( (log_level > 0) && (log_out != NULL) ) {
    va_list arg;

    if (error != 0)
      fprintf  (log_out, "error %i: ", error);

    va_start (arg, format);
    vfprintf (log_out, format, arg);
    va_end (arg);

    fflush (log_out);
  }
}
