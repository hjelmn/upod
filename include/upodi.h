/**
 *   (c) 2002 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.0.2 upodi.h
 *
 *   Internal functions. Do not include upodi.h in any end software.
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
#ifndef __UPODI_H
#define __UPODI_H

#include <sys/types.h>

#include "upod.h"

#define DEBUG

#if defined(DEBUG)
#define UPOD_DEBUG printf
#else
#define UPOD_DEBUG
#endif

#if defined (DEBUG_MEMORY)
#define   free(x) do {printf ("freeing %08x from line %i in file %s.\n", x, __LINE__, __FILE__); free(x);} while (0);
#endif
 
/* this definition will not be static in non-alpha release */
#define EMPTYDB "/home/neo/cvs/upod/share/emptydb"

/* some structures to help clarify code */
/* plhms are headers to pyhms */
struct db_plhm {
  u_int32_t plhm;
  u_int32_t header_size;
  u_int32_t num_pyhm;
};

/* tlhms are headers to pihms */
struct db_tlhm {
  u_int32_t tlhm;
  u_int32_t header_size;
  u_int32_t num_tihm;
};

struct db_pihm {
  u_int32_t pihm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t unk[3];
  u_int32_t reference;
};

struct db_pyhm {
  u_int32_t pyhm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t flags;
  u_int32_t num_pihm;
  u_int32_t is_visible;
};

struct db_tihm {
  u_int32_t tihm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t num_dohm;
  u_int32_t identifier;
  u_int32_t type;
  u_int32_t unk0;
  u_int32_t unk1;
  u_int32_t date;
  u_int32_t file_size;

  /* in millisecs */
  u_int32_t duration;

  u_int32_t order;
  u_int32_t encoding;
  u_int32_t unk[2];
  u_int32_t sample_rate;
  u_int32_t volume_adjustment;

  /* in millisecs */
  u_int32_t start_time;
  u_int32_t stop_time;
};

struct db_dohm {
  u_int32_t dohm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t type;
  u_int32_t unk0;
  u_int32_t unk1;
  u_int32_t unk2;
  u_int32_t len;
};

#define UPOD_ERROR(x, format, args...) do {\
  fprintf(stderr, format, ## args);\
  exit(x);\
} while(0);

#define UPOD_NOT_IMPL(s) do {\
  fprintf(stderr, "Error -1: function %s not implemented\n", s);\
  return -1;\
} while(0);

/* libupod/endian.c */
#if defined (__linux__)

#include <endian.h>
#include <byteswap.h>

#elif defined (__darwin__)

#include <machine/endian.h>
#include <architecture/byte_order.h>

#define bswap_32 NXSwapLittleLongToHost
#define bswap_16 NXSwapLittleShortToHost

#endif
void bswap_block (char *ptr, size_t membsize, size_t nmemb);

#if BYTE_ORDER == BIG_ENDIAN
#define long_big_host(x) x
#else
#define long_big_host(x) bswap_32(x)
#endif


tihm_t db_song_info (ipod_t *ipod, u_int32_t mhit_num);

/* libupod/tihm.c */
int db_tihm_search (struct tree_node *entry, u_int32_t tihm_num);
int db_tihm_create (struct tree_node *entry, char *filename, char *path);

tihm_t *tihm_create(tihm_t *tihm, char *filename, char *path, int num);
tihm_t *db_tihm_fill (struct tree_node *entry);
void tihm_free (tihm_t *tihm);

int db_tihm_retrieve (struct tree iTunesDB, struct tree_node **entry,
		      struct tree_node **parent, int tihm_num);

/* libupod/pihm.c */
int db_pihm_search (struct tree_node *entry, u_int32_t tihm_num);
int db_pihm_create (struct tree_node *entry, u_int32_t tihm_num, u_int32_t junk);

/* libupod/unicode.c */
void char_to_unicode (char *dst, char *src, size_t src_length);
void unicode_to_char (char *dst, char *src, size_t src_length);

/* libupod/dohm.c */
dohm_t *dohm_create (tihm_t *tihm);
void    dohm_destroy(tihm_t *tihm);

int     db_dohm_create_generic (struct tree_node *entry, size_t size, int junk);
int     db_dohm_create_eq (struct tree_node *entry, int eq);

struct tree_node *db_dohm_search(struct tree_node *tihm_entry, int dohm_num);
dohm_t *db_dohm_fill (struct tree_node *entry);

void    dohm_free (dohm_t *dohm, int num_dohm);

/* path seperator should be : */
int path_mac_to_unix (char *mac_path, char *unix_dst);
/* path seperator should be / */


/* libupod/mp3.c */

/* libupod/playlist.c */

char *basename(char *);
void *memmem(void *haystack, size_t haystack_size, void *needle, size_t needle_size);

void db_free_tree (struct tree_node *ptr);

#endif /* __UPODI_H */
