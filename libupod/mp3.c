/**
 *   (c) 2002 Nathan Hjelm <hjelmn@unm.edu>
 *   v0.1.0a mp3.c 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "itunesdbi.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>

/* from mpg123 */
#include "genre.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "mp3tech.h"

/*
  Fills in the mp3_file structure and returns the file offset to the 
  first MP3 frame header.  Returns >0 on success; -1 on error.
   
  This routine is based in part on MP3Info 0.8.4.
  MP3Info 0.8.4 was created by Cedric Tefft <cedric@earthling.net> 
  and Ricardo Cerqueira <rmc@rccn.net>
*/
static int get_mp3_header_info (FILE *fh, char *file_name, tihm_t *tihm) {
  int scantype=SCAN_QUICK, fullscan_vbr=1;
  mp3info mp3;

  memset(&mp3,0,sizeof(mp3info));
  mp3.filename=file_name;

  mp3.file = fh;

  get_mp3_info(&mp3, scantype, fullscan_vbr);
  if(!mp3.header_isvalid) {
    get_mp3_info (&mp3, SCAN_FULL, 1);

    if (!mp3.header_isvalid)
      return -1;
  }
  
  /* the ipod wants time in thousands of seconds */
  tihm->time        = mp3.seconds * 1000;
  tihm->samplerate  = header_frequency(&mp3.header);  

  tihm->vbr = mp3.vbr;

  if (mp3.vbr)
    tihm->bitrate   = (int)mp3.vbr_average;
  else
    tihm->bitrate   = header_bitrate(&mp3.header); 

  fseek (fh, 0, SEEK_SET);

  return 0;
}

/*
  mp3_fill_tihm:

  fills a tihm structure for adding a mp3 to the iTunesDB.

  Returns:
   < 0 if any error occured
     0 if successful
*/
int mp3_fill_tihm (u_int8_t *file_name, tihm_t *tihm){
  struct stat statinfo;
  int ret;
  FILE *fh;

  u_int8_t type_string[] = "MPEG audio file";

  ret = stat(file_name, &statinfo);

  if (ret < 0) {
    perror("mp3_fill_tihm");
    return -1;
  }

  tihm->size = statinfo.st_size;

  if ((fh = fopen(file_name,"r")) == NULL )
    return errno;

  if (get_id3_info(fh, file_name, tihm) < 0) {
    fclose (fh);

    return -1;
  }

  if (get_mp3_header_info(fh, file_name, tihm) < 0) {
    fclose (fh);

    return -1;
  }

  fclose (fh);

  dohm_add (tihm, type_string, strlen (type_string), "UTF-8", IPOD_TYPE);

  return 0;
}
