/**
 *   (c) 2003-2005 Nathan Hjelm <hjelmn@users.sourceforge.net>
 *   v0.2.0 itunesdbi.h
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
#include <sys/types.h>
#include <wand/magick_wand.h>
#include "itunesdb.h"

void db_log (ipoddb_t *itunesdb, int error, char *format, ...);

#if defined (DEBUG_MEMORY)
#define   free(x) do {printf ("freeing %08x from line %i in file %s.\n", x, __LINE__, __FILE__); free(x);} while (0);
#endif

/* some structures to help clarify code */
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

/* plhms are list headers to pyhms */
/* iTunesDB Playlist Header */
struct db_plhm {
  u_int32_t plhm;
  u_int32_t header_size;
  u_int32_t num_pyhm;

  u_int32_t unk0[20]; /* 0's */
};

/* tlhms are list headers to tihms */
/* iTunesDB Track List Header */
struct db_tlhm {
  u_int32_t tlhm;
  u_int32_t header_size;
  u_int32_t num_tihm;

  u_int32_t unk0[20]; /* 0's */
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
  u_int32_t iihm_id1;
  u_int32_t iihm_id2;
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

  u_int16_t height;
  u_int16_t width;
  u_int32_t unk3[3];

  u_int32_t unk4[7];
};

/* Image Item Header */
struct db_iihm {
  u_int32_t iihm;
  u_int32_t header_size;
  u_int32_t record_size;
  u_int32_t num_thumbs;

  u_int32_t identifier;
  u_int32_t id1;
  u_int32_t id2;
  u_int32_t unk0;

  u_int32_t unk1[4];

  u_int32_t source_size;
  u_int32_t unk2[3];

  u_int32_t unk3[22];
};

/* Image List Header */
struct db_ilhm {
  u_int32_t ilhm;
  u_int32_t header_size;
  u_int32_t num_images;

  u_int32_t unk0[20];
};

/* Album List Header */
struct db_alhm {
  u_int32_t alhm;
  u_int32_t header_size;
  u_int32_t num_albums;
  
  u_int32_t unk0[20];
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

/* Fileid List Header */
struct db_flhm {
  u_int32_t flhm;
  u_int32_t header_size;
  u_int32_t num_files;
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

static u_int32_t string_to_int (unsigned char *string) {
  return string[0] << 24 | string[1] << 16 | string[2] << 8 | string[3];
}


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

#define bswap_32 NXSwapLittleLongToHost
#define bswap_16 NXSwapLittleShortToHost

#endif
void bswap_block (char *ptr, size_t membsize, size_t nmemb);

#if BYTE_ORDER == BIG_ENDIAN
#define long_big_host(x) x
#else
#define long_big_host(x) bswap_32(x)
#endif

/* 
   Function declarations divided by source file

   (Note: there are more declarations in itunesdb.h)
*/

/* db.c */
void    db_free_tree (tree_node_t *ptr);

int     db_attach    (tree_node_t *parent, tree_node_t *new_child);
int     db_attach_at (tree_node_t *parent, tree_node_t *new_child, int index);
int     db_detach    (tree_node_t *parent, int child_num, tree_node_t **entry);
int     db_node_allocate (tree_node_t **entry, unsigned long type,
			  size_t size, int subtree);

/* tihm.c */
int     db_tihm_search   (tree_node_t *entry, u_int32_t tihm_num);
int     db_tihm_create   (tree_node_t **entry, tihm_t *tihm);
tihm_t *tihm_create      (tihm_t *tihm, char *filename, char *path, int num);
tihm_t *db_tihm_fill     (tree_node_t *entry);
int     db_tihm_retrieve (ipoddb_t *itunesdb, tree_node_t **entry,
			  tree_node_t **parent, int tihm_num);
int     tihm_fill_from_file (tihm_t *tihm, char *path, u_int8_t *ipod_path,
			     size_t path_len, int stars, int tihm_num,
			     int ipod_use_unicode_hack);
void    tihm_free        (tihm_t *tihm);

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

/* abhm.c */
int db_abhm_create (tree_node_t **entry);
int db_abhm_dohm_attach (tree_node_t *entry, tree_node_t *dohm);
int db_abhm_aihm_attach (tree_node_t *entry, tree_node_t *aihm);

/* dohm.c */
int db_dohm_retrieve (tree_node_t *tihm_header, tree_node_t **dohm_header,
                      int dohm_type);
dohm_t *dohm_create     (tihm_t *tihm, int data_type);
void    dohm_destroy    (tihm_t *tihm);
int     db_dohm_create_generic (tree_node_t **entry, size_t size, int type);
int     db_dohm_create_pihm (tree_node_t **entry, int order);
int     db_dohm_create_eq (tree_node_t **entry, int eq);
int     db_dohm_create (tree_node_t **entry, dohm_t dohm, int string_header_size);
dohm_t *db_dohm_fill    (tree_node_t *entry);
void    dohm_free       (dohm_t *dohm, int num_dohm);
/* Operations on a wierd dohm */
int db_dohm_itunes_create (tree_node_t **entry);
int db_dohm_itunes_show (tree_node_t *entry, int column_id, int column_width);
int db_dohm_itunes_hide (tree_node_t *entry, int column_id);


/* dshm.c */
int db_dshm_retrieve (ipoddb_t *itunesdb, tree_node_t **dshm_header,
		      int type);
int db_dshm_create (tree_node_t **entry, int type);

/* unicode.c */
void char_to_unicode (u_int16_t *dst, u_int8_t *src, size_t src_length);
void unicode_to_char (u_int8_t *dst, u_int16_t *src, size_t src_length);
void to_unicode (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
		 size_t src_len, char *src_encoding);
void to_unicode_hack (u_int16_t **dst, size_t *dst_len, u_int8_t *src,
		      size_t src_len, char *src_encoding);
void unicode_to_utf8 (u_int8_t **dst, size_t *dst_len, u_int16_t *src,
		      size_t src_len);
int unicodencasecmp (u_int8_t *string1, size_t string1_len, u_int8_t *string2, size_t string2_len);

int mp3_fill_tihm (u_int8_t *, tihm_t *);
int aac_fill_tihm (char *, tihm_t *);

/* id3.c */
int get_id3_info (FILE *fd, char *file_name, tihm_t *tihm);

/* playlist.c */
int db_playlist_retrieve (ipoddb_t *, struct db_plhm **, tree_node_t **, int, tree_node_t **);

/* inhm.c */
int db_inhm_create (tree_node_t **entry, int file_id, char *file_name,
		    char *rel_mac_path, MagickWand *magick_wand);

/* iihm.c */
int db_iihm_create (tree_node_t **entry, int identifier, int id1, int id2);
int db_iihm_search (tree_node_t *entry, u_int32_t iihm_identifier);
int db_iihm_retrieve (ipoddb_t *photodb, tree_node_t **entry,
                      tree_node_t **parent, int iihm_identifier);

/* fihm.c */
int db_fihm_create (tree_node_t **entry, unsigned int file_id);
int db_fihm_register (ipoddb_t *photodb, char *file_name, unsigned long file_id);

#endif /* __ITUNESDBI_H */
