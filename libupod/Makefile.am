libdir = $(prefix)/lib

INCLUDES += -I$(top_srcdir)/include
CFLAGS += -ansi `pkg-config --cflags glib` -g3
LDFLAGS += `pkg-config --libs glib`

lib_LTLIBRARIES = libupod.la
libupod_la_SOURCES = aac.c abhm.c aihm.c album.c checks.c cksum.c \
		     create.c db.c db_lookup.c dohm.c dshm.c endian.c \
		     fihm.c id3.c iihm.c image_list.c inhm.c ipod.c \
		     log.c mp3.c pihm.c playlist.c pyhm.c smart_playlist.c \
		     song_list.c sysinfo.c tihm.c unicode.c

libupod_la_LDFLAGS = -version-info 3:1:0
