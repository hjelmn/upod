/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1 database.c
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

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/errno.h>

#ifdef __powerpc__
#include <endian.h>
#include <byteswap.h>
#endif

#include "hexdump.c"

#include "upodi.h"
#include "endian.c"

/* this function corrects the endianness of the data in memory */
int db_swapints (ipod_t *ipod) {
  //#if defined (__powerpc__)
  char *ptr = ipod->itunesdb;
  u_int32_t *iptr = (u_int32_t *)ptr;
  int i, j;

  for (i = 0 ; i < ipod->db_size_current ; ) {

    /* just correct the data in the cell header */
    for (j = 0 ; j < (iptr[1] - 12) ; j += 4)
      swap32(&iptr[3 + j/4]);

    if (!strstr(ptr, "mhod")) {
      /* not an mhod, just add header size */
      i += iptr[1];
      ptr += iptr[1];
    } else {
      /* an mhod, we don't want to swap long chars so skip rest of cell */
      i += iptr[2];
      ptr += iptr[2];
    }

    /* finally fix these pointers */
    swap32(&iptr[1]);
    swap32(&iptr[2]);

    iptr = (u_int32_t *)ptr;
  }
  //#endif

  if (ipod->debug)
    printf("do_swapints: memory ready for use on ppc\n");

  return 0;
}

int db_to_memory (ipod_t *ipod) {
  struct stat statinfo;

  int i = 0;

  /* find out db size */
  fstat(ipod->db_fd, &statinfo);
  ipod->db_size_current = statinfo.st_size;

  /* allocate or reallocate memory for database */
  if (ipod->itunesdb)
    realloc(ipod->itunesdb, ipod->db_size_current);
  else
    ipod->itunesdb = (char *)malloc(ipod->db_size_current);

  if (ipod->itunesdb == NULL) {
    if (ipod->debug)
      perror("db_to_memory");
    
    return errno;
  } else {
    if (ipod->debug)
      printf("db_to_memory: memory successfully malloced or realloced\n");
  }

  lseek(ipod->db_fd, 0, SEEK_SET);
  
  if (read(ipod->db_fd, ipod->itunesdb, ipod->db_size_current) < 0) {
    if (ipod->debug)
      perror("db_to_memory");

    return errno;
  } else {
    if (ipod->debug)
      printf("db_to_memory: db successfully loaded into memory\n");
  }

  db_swapints (ipod);

  return 0;
}

/*
int memory_to_db (struct ipod_instance prt_ipod) {
  ipod_t ipod = (ipod_t *)ptr_ipod;

  db_swapints (ipod);
}
*/

int main(int argc, char *argv[]) {
  ipod_t ipod;

  if (argc != 2)
    return -1;

  ipod.itunesdb = NULL;
  ipod.db_fd = open(argv[1], O_RDONLY);
  ipod.debug = 2;

  db_to_memory(&ipod);

  free(ipod.itunesdb);
  close(ipod.db_fd);

  return 0;
}
