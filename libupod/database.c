/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.1 database.c
 *
 *   Routines for accessing the iPod's iTunesDB
 *
 *   Changes:
 *     - handles byte conversion for powerpc in both directions
 *     - db_remove works now
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

#include <time.h>

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

typedef union _superptr {
  char      *ptr;
  u_int32_t *iptr;
} superptr_t;

/* this function corrects the endianness of the data in memory (if needbe) */
static int db_swapints (ipod_t *ipod, int endian) {
#if defined (__powerpc__)
  superptr_t location;
  int i, j;

  location.ptr = ipod->itunesdb;

  if ((endian == BIG_ENDIAN) && (strstr(ipod->itunesdb, "dbhm"))) {
    UPOD_DEBUG("db_swapints: called with BIG_ENDIAN and db is already BIG_ENDIAN\n");
    return 0;
  } else if ((endian == LITTLE_ENDIAN) && (strstr(ipod->itunesdb, "mhbd"))) {
    UPOD_DEBUG("db_swapints: called with LITTLE_ENDIAN and db is already LITTLE_ENDIAN\n");
    return 0;
  }
	  
  for (i = 0 ; i < ipod->db_size_current ; ) {
    int *tmp = location.iptr;

    if (endian == BIG_ENDIAN) {
      swap32(&(tmp[0]));
      swap32(&(tmp[1]));
      swap32(&(tmp[2]));
    }

    /* just correct the data in the cell header */
    for (j = 0 ; j < (location.iptr[1] - 12) ; j += 4)
      location.iptr[3 + j/4] = bswap_32(location.iptr[3 + j/4]);

    if (!strstr(location.ptr, "dohm")) {
      /* not an dohm, just add header size */

      i += location.iptr[1];
      location.ptr += location.iptr[1];
    } else {
      /* for some reason this is not included in the header size */
      /* swap next 7 words */
      for (j = (location.iptr[1] - 12) ; j < 28 ; j += 4)
	swap32(&(location.iptr[3 + j/4]));

      /* an dohm, we don't want to swap long chars so skip rest of cell */
      i += location.iptr[2];
      location.ptr += location.iptr[2];
    }

    if (endian == LITTLE_ENDIAN) {
      swap32(&(tmp[0]));
      swap32(&(tmp[1]));
      swap32(&(tmp[2]));
    }
  }
  
  if (endian == BIG_ENDIAN)
    UPOD_DEBUG("do_swapints: memory ready for use on ppc\n");
  else
    UPOD_DEBUG("do_swapints: memory ready to write to a file\n");
#endif

  return 0;
}

int db_to_memory (ipod_t *ipod) {
  struct stat statinfo;
  struct timeval srt,stp;
  int i = 0;

  /* does nothing very useful */
  if (ipod->debug) {
    gettimeofday(&srt, NULL);
  }

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
      UPOD_DEBUG("db_to_memory: memory successfully malloced or realloced (0x%08x Bytes)\n",
		 ipod->db_size_current);
  }

  lseek(ipod->db_fd, 0, SEEK_SET);
  
  if (read(ipod->db_fd, ipod->itunesdb, ipod->db_size_current) < 0) {
    if (ipod->debug)
      perror("db_to_memory");

    return errno;
  } else {
    UPOD_DEBUG("db_to_memory: db successfully loaded into memory\n");
  }

  /* swap to bigendian if needbe */
  db_swapints (ipod, BIG_ENDIAN);

  if (ipod->debug) {
    gettimeofday(&stp, NULL);
    printf("db loaded in %i sec %i msec\n", stp.tv_sec - srt.tv_sec,
	   stp.tv_usec - srt.tv_usec);
  }

  return 0;
}

/* create a new empty database */
int db_create (ipod_t *ipod) {
  struct stat statinfo;
  superptr_t *tmp;
  int fd;

  /* i think this is a bit easier to justify than hardcoding\
     the values of an empty db into libupod */
  if ((fd = open(EMPTYDB, O_RDONLY)) < 0) {
    UPOD_ERROR(-20, "db_create: could not open empty db %s.\n");
    perror("db_create:");
    return -20;
  }

  fstat(fd, &statinfo);
  ipod->db_size_current = statinfo.st_size;

  /* allocate or reallocate memory for database */
  if (ipod->itunesdb)
    realloc(ipod->itunesdb, ipod->db_size_current);
  else
    ipod->itunesdb = (char *)malloc(ipod->db_size_current);

  if (read(fd, ipod->itunesdb, ipod->db_size_current) < 0) {
    if (ipod->debug)
      perror("db_create");

    return errno;
  } else {
    UPOD_DEBUG("db_create: db successfully loaded into memory\n");
  }

  /* swap to bigendian if needbe */
  db_swapints (ipod, BIG_ENDIAN);
  
  return close(fd);
}

/* writing db is a future feature */
int memory_to_db (ipod_t *ipod) {
  /* database should be in little-endian format before
     being written */
  db_swapints (ipod, LITTLE_ENDIAN);

  if (lseek(ipod->db_fd, 0, SEEK_SET)) {
    perror("memory_to_db");
    db_swapints(ipod, BIG_ENDIAN);
    return errno;
  }

  if (write(ipod->db_fd, ipod->itunesdb, ipod->db_size_current)) {
    perror("memory_to_db");
    db_swapints(ipod, BIG_ENDIAN);
    return errno;
  }

  return db_swapints (ipod, BIG_ENDIAN);
}

int db_add (ipod_t *ipod, char *src) {
  char *dst = ipod->insertion_point;
  int est_size;

  song_ref_t *new_entry;
  superptr_t tmp;
  
  UPOD_DEBUG("db_add: adding file to iTunesDB\n");

  new_entry = malloc(sizeof(song_ref_t));
  if (ref_fill_mp3(src, new_entry) != 0) {
    UPOD_ERROR(-5, "db_add: ref_fill_mp3 returned non-zero\n");
    return -1;
  }
    
  /* calculate the size of the new db entry */

  
  if (dst == NULL) {
    free(new_entry);
    UPOD_ERROR(-10, "No insertion point\n");
    return -10;
  }

  tmp.ptr = malloc(est_size);

  free(tmp.ptr);

  UPOD_DEBUG("db_add: song added to the iTunesDB\n");
  return 0;
}

int db_remove (ipod_t *ipod, song_ref_t *ref) {
  superptr_t tmp;
  int remainder;
  int i, cell_size;

  tmp.ptr = ref->db_location;
  remainder = ipod->db_size_current - 
    (ref->db_location - tmp.ptr) - tmp.iptr[2];

  cell_size = tmp.iptr[2];
  fprintf(stderr, "cell_size = 0x%08x\n", cell_size);
  ipod->db_size_current -= cell_size;

  ref->prev->next = ref->next;
  ref->next->prev = ref->prev;

  memcpy(ref->db_location, ref->db_location + cell_size, remainder);

  realloc (ipod->itunesdb, ipod->db_size_current);

  for (i = 0 ; i < ref->dohm_num ; i++)
    free(ref->dohm_entries[i].data);

  free(ref->dohm_entries);
  free(ref);

  /* update the size of the db */
  tmp.iptr[2] = ipod->db_size_current;

  /* update the size of the playlist holder */
  tmp.iptr[2 + tmp.iptr[1]] -= cell_size;
  /* update the size of the playlist -- TODO -- this looks nasty, 
   it needs to be cleaned up */
  tmp.iptr[2 + tmp.iptr[1] + tmp.iptr[1 + tmp.iptr[1]]] -= cell_size;

  /* now remove the song's entry from the master playlist */

  UPOD_DEBUG("db_remove: successfull\n");
}

char *db_get_path (ipod_t *ipod, song_ref_t *ref) {
  int i;

  for (i = 0 ; i < ref->dohm_num ; i++) {
    if (ref->dohm_entries[i].type == 2)
      return ref->dohm_entries[i].data;
  }
}

#define TYPE_TIHM 0x7469686d
#define TYPE_DOHM 0x6471686d
#define TYPE_DBHM 0x6462686d
#define TYPE_DSHM 0x6473686d
#define TYPE_TLHM 0x746c686d
#define TYPE_PLHM 0x706c686d

/*
  lc_to_char:

    Copies a long char from ptr of length length
  and prints the 8 bit char to dst. It also changes
  any occurance of the character ':' to '/' in dst.
 */
void lc_to_char (char *ptr, int length, char *dst) {
  int i;

  for (i = 0 ; i < (length/2) ; i++)
    if (ptr[2 * i] == ':')
      dst[i] = '/';
    else
      dst[i] = ptr[2 * i];

  dst[length/2 + 1] = 0;
}

void char_to_lc (char *ptr, int length, char *dst) {
  int i;

  memset(dst, 0, length * 2);
  printf("%s\n", ptr);
  for (i = length - 1 ; i >= 0 ; i--) {
    if (ptr[i] == '/')
      dst[2 * i] = ':';
    else
      dst[2 * i] = ptr[i];    
  }
}

song_ref_t *build_ref_list (ipod_t *ipod) {
  song_ref_t *head, **current, *prev;
  int i,j;
  superptr_t location;

  location.ptr = ipod->itunesdb;

  UPOD_DEBUG("Entering build_ref_list\n");

  if ((head = calloc(sizeof(song_ref_t), 1)) == NULL) {
    perror("build_ref_list");
    return NULL;
  }

  current = &(head->next);
  prev = head;

  for (i = 0 ; location.iptr[0] != TYPE_PLHM ; i += location.ptr[2]) {
    if (location.iptr[0] == TYPE_TIHM) {
      superptr_t dohm;

      (*current) = calloc(sizeof(song_ref_t), 1);
      
      (*current)->prev = prev;
      (*current)->next = head;

      (*current)->db_location = location.ptr;

      (*current)->size       = location.iptr[9];
      (*current)->time       = location.iptr[10];
      (*current)->bitrate    = location.iptr[14];
      (*current)->samplerate = location.iptr[15] >> 16;

      (*current)->dohm_num = location.iptr[3];
      (*current)->dohm_entries = calloc(sizeof(struct dohm), 
					(*current)->dohm_num);

      // iterate through dohm entries
      dohm.ptr = location.ptr + location.iptr[1];
      for (j = 0 ; j < (*current)->dohm_num ; j++) {
	int length = dohm.iptr[dohm.iptr[1]/4 + 1];

	(*current)->dohm_entries[j].type = dohm.iptr[3];
	(*current)->dohm_entries[j].data = calloc(1, length + 1);

	lc_to_char(dohm.ptr + dohm.iptr[1] + 16, length, 
		   (*current)->dohm_entries[j].data);

	dohm.ptr += dohm.iptr[2];
      }
            
      prev = *current;
      current = &(prev->next);
    }

    if ((location.iptr[0] != TYPE_DBHM) && (location.iptr[0] != TYPE_DSHM)
	&& (location.iptr[0] != TYPE_TLHM) )
      location.ptr += location.iptr[2];
    else
      location.ptr += location.iptr[1];
  }

  head->prev = prev;

  UPOD_DEBUG("finished building reference list.\n");

  return head;
}

void ref_clear(song_ref_t *ref) {
  int i;

  if (ref == NULL)
    return;

  for (i = 0 ; i < ref->dohm_num ; i++)
    free(ref->dohm_entries[i].data);

  free(ref->dohm_entries);

  memset(ref, 0, sizeof(song_ref_t));
}

void free_ref_list(song_ref_t *ref) {
  int i;
  while (ref) {
    song_ref_t *tmp = ref->next;

    ref_clear(ref);

    if (ref->next)
      ref->next->prev = NULL;
    if (ref->prev)
      ref->prev->next = NULL;

    free(ref);
    ref = tmp;
  }
}

char *str_type(int file_type) {
  switch (file_type) {
  case TITLE:
    return "Title";
  case PATH:
    return "Path";
  case ALBUM:
    return "Album";
  case ARTIST:
    return "Artist";
  case GENRE:
    return "Genre";
  case TYPE:
    return "Type";
  case COMMENT:
    return "Comment";
  default:
    return "Unknown";
  }
}

int main(int argc, char *argv[]) {
  ipod_t ipod;
  song_ref_t *head;
  int i, j, fd;

  if (argc != 2)
    return -1;

  ipod.itunesdb = NULL;
  ipod.db_fd = open(argv[1], O_RDONLY);
  ipod.debug = 2;

  db_to_memory(&ipod);

  head = build_ref_list(&ipod);
  for (head = head->next ; head->db_location != NULL ; head = head->next) {
    for (j = 0 ; j < head->dohm_num ; j++) {
      if (head->dohm_entries[j].type == 1)
	//	if (strstr(head->dohm_entries[j].data, "Sober")) {
	  printf("timh! (track information)\n");
	  printf("samplerate: %i   bitrate: %i\n", head->samplerate,
		 head->bitrate);
	  printf("time: %i microseconds   size: %i Bytes\n", head->time,
		 head->size);
	  for (i = 0 ; i < head->dohm_num ; i++)
	    printf("type: %-32s data: %-64s\n",
		   str_type(head->dohm_entries[i].type),
		   head->dohm_entries[i].data);
	  //	  db_remove(&ipod, head);
	  //	}
    }
  }

  free_ref_list(head);
  
  fd = creat ("file2", O_RDWR);

  db_swapints(&ipod, LITTLE_ENDIAN);
  write(fd, ipod.itunesdb, ipod.db_size_current);
  close(fd);
  free(ipod.itunesdb);
  close(ipod.db_fd);

  return 0;
}


/*
int main(int argc, char *argv[]) {
  song_ref_t testv;
  superptr_t tmp, tmp2;
  int i, size_est = 0x9c;

  ref_fill_mp3(argv[1], &testv);
  for (i = 0 ; i < testv.dohm_num ; i++)
    size_est += 0x18 + 0x10 + 2 * strlen(testv.dohm_entries[i].data);

  tmp.ptr = malloc(size_est);
  printf("Estimated size of new entry: %i bytes\n", size_est);
  printf("Block allocated starting: %i\n", tmp.ptr);
  sprintf(tmp.ptr, "timh");
  tmp.iptr[1] = 0x9c;           // header size
  tmp.iptr[2] = size_est;       // cell size
  tmp.iptr[3] = testv.dohm_num; // number of dohm entries
  tmp.iptr[4] = 539;            // record index
  tmp.iptr[9] = testv.size;     // file size
  tmp.iptr[10]= testv.time;     // duration
  tmp.iptr[11]= 1;              // track number
  tmp.iptr[14]= testv.bitrate;  // bitrate
  tmp.iptr[15]= testv.samplerate << 16;

  for (tmp2.ptr = tmp.ptr + 0x9c, i=0 ; i < testv.dohm_num ; i++) {
    sprintf(tmp2.ptr, "dohm");
    printf("dohm %i\n", i);
    tmp2.iptr[1] = 0x18;
    tmp2.iptr[2] = 0x18 + 0x10 + 2 * strlen(testv.dohm_entries[i].data);
    tmp2.iptr[3] = testv.dohm_entries[i].type;
    tmp2.iptr[6] = 0x01;
    tmp2.iptr[7] = 2 * strlen(testv.dohm_entries[i].data);
    char_to_lc(testv.dohm_entries[i].data,
	       strlen(testv.dohm_entries[i].data), (char *)&tmp2.iptr[10]);
    tmp2.ptr += tmp2.iptr[2];
  }

  pretty_print_block(tmp.ptr, size_est);
  free(tmp.ptr);
  ref_clear(&testv);
}
*/
