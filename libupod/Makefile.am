libdir = $(prefix)/lib

INCLUDES = -I$(top_srcdir)/include

lib_LTLIBRARIES = libupod.la

libupod_la_SOURCES = album.c create.c device_info.c endian.c id3.c \
		     inhm.c mp3.c play_counts.c smart_playlist.c tihm.c \
		     abhm.c checks.c db.c dohm.c fihm.c iihm.c \
	             ipod.c otg_playlist.c playlist.c song_list.c unicode.c \
		     aihm.c cksum.c db_lookup.c dshm.c hexdump.c \
		     image_list.c log.c mp4.c pihm.c podcast.c pyhm.c \
		     sysinfo.c itunessd.c genre.c list.c
libupod_la_LDFLAGS = -version-info 3:1:2 -Wl,-multiply_defined -Wl,suppress
