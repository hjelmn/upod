/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.3.1 itunesdbi.h
 *
 *   Internal functions. Do not include ipoddbi.h in any end software.
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
#ifndef __ITUNESDBI_H
#define __ITUNESDBI_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

#if defined(HAVE_LIBWAND)
#include <wand/magick_wand.h>
#endif

#include "itunesdb.h"

void db_log (ipoddb_t *itunesdb, int error, char *format, ...);

#if defined (DEBUG_MEMORY)
#define calloc(x,y) calloc (x,y); printf ("allocating %i member(s) of size %i from line %i in file %s.\n", x, y, __LINE__, __FILE__);
#define free(x) do {printf ("freeing %08x from line %u in file %s.\n", x, __LINE__, __FILE__); free(x);} while (0);
#endif

/* some structures to help keep the code clean */
struct db_generic {
  u_int32_t type;
  u_int32_t cell_size;
  u_int32_t subtree_size;
  u_int32_t num_dohm; /* tihm, pyhm */
};

/* iTunesDB Database Header */
struct db_dbhm {
  u_int32_t dbhm;
  u_int32_t header_size;
  u_int32_t db_size;
  u_int32_t unk0;

  u_int32_t itunes_version;
  u_int32_t num_dshm;
  u_int32_t unk3;
  u_int32_t unk4;

  u_int32_t unk5; /* Always seems to be 1 */
  u_int32_t unk6;
  u_int32_t unk7;
  u_int32_t unk8;

  u_int32_t unk9[14]; /* 0's */
};

struct db_dshm {
  u_int32_t dshm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t type;

  u_int32_t unk0[20]; /* 0's */
};

/* List headers */
struct db_lhm {
  u_int32_t tlhm;
  u_int32_t header_size;
  u_int32_t list_entries;

  u_int32_t unk0[20]; /* 0's */
};

/* iTunesDB Track List Header */
typedef struct db_lhm db_tlhm_t;
/* iTunesDB Playlist Header */
typedef struct db_lhm db_plhm_t;

/* ArtworkDB Image List */
typedef struct db_lhm db_ilhm_t;
/* Photo Database Album List */
typedef struct db_lhm db_alhm_t;
/* ArtworkDB File ID List */
typedef struct db_lhm db_flhm_t;


/* Playlist Header */
struct db_pyhm {
  u_int32_t pyhm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t num_dohm;

  u_int32_t num_pihm;
  u_int32_t is_visible;
  u_int32_t creation_date; /* secs since epoch? */
  u_int32_t playlist_id;

  u_int32_t unk0; /* 0 */
  u_int32_t unk1; /* 0 */
  u_int32_t unk2; /* 1 */
  u_int32_t unk3; /* 1 */

  u_int32_t unk4[15]; /* 0's */
};

/* iTunesDB Playlist Item Header */
struct db_pihm {
  u_int32_t pihm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t unk0;

  u_int32_t unk1;
  /* matches an the first integer after the header of
     the following dohm. */
  u_int32_t order;
  u_int32_t reference;
  u_int32_t date_added;

  u_int32_t unk3[11]; /* 0's */
};


/* Track Item Header */
struct db_tihm {
  u_int32_t tihm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t num_dohm;

  u_int32_t identifier;
  u_int32_t type;
  u_int32_t unk0;
  u_int32_t flags; /* start rating contained in the least sig. byte mul. by 0x14 */

  u_int32_t creation_date;
  u_int32_t file_size;
  u_int32_t duration;   /* in millisecs */
  u_int32_t order;

  u_int32_t album_tracks; /* number of tracks on album */
  u_int32_t year;
  u_int32_t bit_rate;
  u_int32_t sample_rate;

  u_int32_t volume_adjustment;
  u_int32_t start_time; /* in millisecs */
  u_int32_t stop_time;  /* in millisecs */
  u_int32_t sound_check;

  u_int32_t num_played[2]; /* no idea why there are two of these */
  u_int32_t last_played_date;
  u_int32_t disk_num;

  u_int32_t disk_total;
  u_int32_t user_id; /* for iTMS songs */
  u_int32_t modification_date;
  u_int32_t bookmark_time;

  /* These ids might be an image checksum to avoid duplicates */
  u_int64_t iihm_id;
  u_int32_t unk1;        /* includes Beats Per Minute, Checked, etc */
  u_int32_t has_artwork; /* usually 0xffff0001 BE */

  u_int32_t unk2[29];
};

/* Photo Database */
/* ArtworkDB Database Header */
struct db_dfhm {
  u_int32_t dfhm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t unk0;

  u_int32_t unk1;
  u_int32_t num_dshm;
  u_int32_t unk2;
  u_int32_t next_iihm;

  u_int32_t unk3;
  u_int32_t unk4;
  u_int32_t unk5;
  u_int32_t unk6;

  u_int32_t unk7; /* usually 1 */
  u_int32_t unk8;
  u_int32_t unk9;
  u_int32_t unk10;

  u_int32_t unk11[17];
};

/* Image Item Header */
/* This structure might have problems on machines
   with 64-bit alignment. */
struct db_iihm {
  u_int32_t iihm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t num_thumbs;

  /* Internal Database identifier */
  u_int32_t identifier;

  /* Identifier shared with iTunesDB */
  u_int64_t id;

  u_int32_t unk0;

  u_int32_t unk1[4];

  u_int32_t source_size;
  u_int32_t unk2[3];

  u_int32_t unk3[22];
};

/* Thumbnail Header */
struct db_inhm {
  u_int32_t inhm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t num_dohm;

  u_int32_t file_id;
  u_int32_t file_offset;
  u_int32_t image_size;
  u_int32_t unk2;

  u_int16_t width;
  u_int16_t height;
  u_int32_t unk3[3];

  u_int32_t unk4[7];
};

/* Album Header */
struct db_abhm {
  u_int32_t abhm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t num_dohm;
  
  u_int32_t num_aihm;
  u_int32_t unk2;
  u_int32_t unk3;
  u_int32_t unk4;
  
  u_int32_t unk5[4];

  u_int32_t unk6[3];
  u_int32_t unk7;

  u_int32_t unk8[21];
};

/* Album Item Header */
struct db_aihm {
  u_int32_t aihm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t unk0;

  u_int32_t reference;
  u_int32_t unk2[3];

  u_int32_t unk3[2];
};

/* Fileid Item Header */
struct db_fihm {
  u_int32_t fihm;
  u_int32_t header_size;
  u_int32_t unk0;

  u_int32_t unk1;
  u_int32_t file_id;
  u_int32_t file_size;
  u_int32_t unk2;

  u_int32_t unk3[24];
};

/* Data Object Header */
struct db_dohm {
  u_int32_t dohm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t type;

  u_int32_t unk0;
  u_int32_t unk1;
};

struct db_string_dohm {
  u_int32_t dohm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t type;

  u_int32_t unk0;
  u_int32_t unk1;
  u_int32_t unk2;
  u_int32_t unk3;

  u_int32_t unk4;
  u_int32_t unk5;
};

/* When format == 1 the string is encoded as UTF-8 */
struct string_header_12 {
  u_int32_t string_length;
  u_int32_t format;
  u_int32_t unk1;
};

struct string_header_16 {
  u_int32_t unk0;
  u_int32_t string_length;
  u_int32_t format;
  u_int32_t unk2;
};

struct db_wierd_dohm {
  u_int32_t dohm;
  u_int32_t header_size; /* 0x018 */
  u_int32_t record_size; /* 0x288 */
  u_int32_t type;        /* 0x064 */

  u_int32_t unk0;
  u_int32_t unk1;
  u_int32_t unk2;
  u_int32_t unk3;

  u_int32_t unk4;
  u_int32_t unk5;
  u_int32_t unk6;
  u_int32_t unk7;
  
  /* itunes display of playlists */
  /* a show entry is:
     0xnn00xxxx
     where nn is one of enum show_entries
     and xxxx is the width of the column
  */

  /*
    contents of shows[0]

    u_int32_t num_show; number of entries shown
    u_int32_t unk8;
    u_int32_t show0;
    u_int32_t unk9;
  */
  struct __shows__ {
    u_int32_t unk10[2];
    u_int32_t show;
    u_int32_t unk11;
  } shows[37];

  u_int32_t unk12;
  u_int32_t unk13;
};

u_int32_t string_to_int (unsigned char *string);


typedef struct tree_node {
  struct tree_node *parent;
  
  u_int8_t *data;
  int data_isalloced;
  size_t data_size;
  
  int num_children;
  struct tree_node **children;
  
  /* Only affects dohm entries containing unicode string. */
  int string_header_size;
} tree_node_t;


/* iTunesDB specific */
#define DBHM string_to_int("dbhm")

/* Songs */
#define TLHM string_to_int("tlhm")
#define TIHM string_to_int("tihm")

/* Playlists */
#define PLHM string_to_int("plhm")
#define PIHM string_to_int("pihm")
#define PYHM string_to_int("pyhm")



/* iPod photo ArtworkDB specific */
#define DFHM string_to_int("dfhm")

/* Images */
#define ILHM string_to_int("ilhm")
#define IIHM string_to_int("iihm")
#define INHM string_to_int("inhm")

/* Albums */
#define ALHM string_to_int("alhm")
#define ABHM string_to_int("abhm")
#define AIHM string_to_int("aihm")

/* Files? */
#define FLHM string_to_int("flhm")
#define FIHM string_to_int("fihm")



/* Common */
#define DSHM string_to_int("dshm")
#define DOHM string_to_int("dohm")


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

#define bswap_32 NXSwapLong
#define bswap_16 NXSwapShort
#define bswap_64 NXSwapLongLong

#endif
void bswap_block (void *ptr, size_t membsize, size_t nmemb);

#if BYTE_ORDER == BIG_ENDIAN
#define big16_2_arch16(x) x
#define big32_2_arch32(x) x
#define big64_2_arch64(x) x

#define little16_2_arch16(x) bswap_16(x)

#define arch16_2_little16(x) bswap_16(x)
#define arch32_2_little32(x) bswap_32(x)
#define arch64_2_little64(x) bswap_64(x)

#define UTF_ENC "UTF-16BE"
#else
#define big16_2_arch16(x) bswap_16(x)
#define big32_2_arch32(x) bswap_32(x)
#define big64_2_arch64(x) bswap_64(x)

#define little16_2_arch16(x) x

#define arch16_2_little16(x) x
#define arch32_2_little32(x) x
#define arch64_2_little64(x) x

#define UTF_ENC "UTF-16LE"
#endif

/* 
   Function declarations divided by source file

   (Note: there are more declarations in itunesdb.h)
*/

/* db.c */
void    db_free_tree (tree_node_t *ptr);

int db_attach    (tree_node_t *parent, tree_node_t *new_child);
int db_attach_at (tree_node_t *parent, tree_node_t *new_child, int index);
int db_detach    (tree_node_t *parent, int child_num, tree_node_t **entry);
int db_node_allocate (tree_node_t **entry, unsigned long type, size_t size, int subtree);

/* tihm.c */
int db_tihm_search (tree_node_t *entry, u_int32_t tihm_num);
int db_tihm_create (tree_node_t **entry, tihm_t *tihm, int flags);
int db_tihm_fill (tree_node_t *tihm_header, tihm_t *tihm);
int db_tihm_retrieve (ipoddb_t *itunesdb, tree_node_t **entry, tree_node_t **parent, int tihm_num);
int db_tihm_get_sorted_indices (ipoddb_t *itunesdb, int sort_by, u_int32_t **indices, u_int32_t *num_indices);

tihm_t *tihm_create (tihm_t *tihm, char *filename, char *path, int num);
int tihm_fill_from_file (tihm_t *tihm, char *path, u_int8_t *ipod_path, int stars, int tihm_num);
void    tihm_free (tihm_t *tihm);


/* pihm.c */
int     db_pihm_search   (tree_node_t *entry, u_int32_t tihm_num);
int     db_pihm_create   (tree_node_t **entry, u_int32_t tihm_num,
			  u_int32_t order);

/* aihm.c */
int db_aihm_search (struct tree_node *entry, u_int32_t image_id);
int db_aihm_create (struct tree_node **entry, u_int32_t image_id);


/* pyhm.c */
int db_pyhm_create (tree_node_t **entry, int is_visible);
int db_pyhm_set_id (tree_node_t *entry, int id);
int db_pyhm_dohm_attach (tree_node_t *entry, tree_node_t *dohm);
int db_pyhm_dohm_detach (tree_node_t *pyhm_header, int index, tree_node_t **store);


/* abhm.c */
int db_abhm_create (tree_node_t **entry);
int db_abhm_dohm_attach (tree_node_t *entry, tree_node_t *dohm);
int db_abhm_aihm_attach (tree_node_t *entry, tree_node_t *aihm);


/* dohm.c */
dohm_t *dohm_create     (tihm_t *tihm, int data_type);
void    dohm_destroy    (tihm_t *tihm);
void    dohm_free       (dohm_t *dohm, int num_dohm);


int db_dohm_retrieve (tree_node_t *entry, tree_node_t **dohm_header, int dohm_type);
int db_dohm_compare (tree_node_t *dohm_header1, tree_node_t *dohm_header2);
int db_dohm_get_string (tree_node_t *dohm_header, u_int8_t **str);
int db_dohm_fill (tree_node_t *entry, dohm_t **dohms);
int db_dohm_create_generic (tree_node_t **entry, size_t size, int type);
int db_dohm_create_pihm (tree_node_t **entry, int order);
int db_dohm_create_eq (tree_node_t **entry, u_int8_t eq);
int db_dohm_index_create (tree_node_t **entry, int sort_by, int num_tracks, u_int32_t tracks[]);
int db_dohm_create (tree_node_t **entry, dohm_t dohm, int string_header_size, int flags);

/* Operations on a wierd dohm */
int db_dohm_itunes_create (tree_node_t **entry);
int db_dohm_itunes_show (tree_node_t *entry, int column_id, int column_width);
int db_dohm_itunes_hide (tree_node_t *entry, int column_id);


/* dshm.c */
int db_dshm_retrieve (ipoddb_t *itunesdb, tree_node_t **dshm_header, int type);
int db_dshm_create (tree_node_t **entry, int type);


/* unicode.c */
void to_unicode (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
		 size_t src_len, char *src_encoding, char *dst_encoding);
void to_unicode_hack (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
		      size_t src_len, char *src_encoding);
int unicodencasecmp (u_int8_t *string1, size_t string1_len, u_int8_t *string2, size_t string2_len);
void to_utf8 (u_int8_t **dst, u_int8_t *src, size_t src_len, char *encoding);

int mp3_fill_tihm (u_int8_t *, tihm_t *);
int aac_fill_tihm (char *, tihm_t *);

/* id3.c */
int get_id3_info (FILE *fd, char *file_name, tihm_t *tihm);

/* playlist.c */
int db_playlist_retrieve (ipoddb_t *, db_plhm_t **, tree_node_t **, int, tree_node_t **);
int db_playlist_strip_indices (ipoddb_t *itunesdb);
int db_playlist_add_indices (ipoddb_t *itunesdb);

/* inhm.c */
#if defined(HAVE_LIBWAND)
int db_inhm_create (tree_node_t **entry, int file_id, char *file_name,
		    char *rel_mac_path, MagickWand *magick_wand);
#endif

/* iihm.c */
int db_iihm_create (tree_node_t **entry, int identifier, u_int64_t id);
int db_iihm_search (tree_node_t *entry, u_int32_t iihm_identifier);
int db_iihm_retrieve (ipoddb_t *photodb, tree_node_t **entry,
                      tree_node_t **parent, int iihm_identifier);

/* fihm.c */
int db_fihm_create (tree_node_t **entry, unsigned int file_id);
int db_fihm_register (ipoddb_t *photodb, char *file_name, unsigned long file_id);


/* crc */
u_int32_t upod_crc32 (u_int8_t *buf, size_t length);
u_int64_t upod_crc64 (u_int8_t *buf, size_t length);

/* sysinfo.c */
int sysinfo_read (ipod_t *ipod, char *filename);

/* db_lookup.c */
int db_lookup_image (ipoddb_t *photodb, u_int64_t id);


int get_uint24 (unsigned char *buf, int block);
#endif /* __ITUNESDBI_H */
