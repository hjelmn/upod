libdir = $(prefix)/lib
EXTRA_DIST = database.c endian.c hexdump.c ipod.c mp3.c playlists.c
INCLUDES += -I$(top_srcdir)/include

lib_LTLIBRARIES = libupod.la
libupod_la_SOURCES = database.c endian.c hexdump.c ipod.c mp3.c \
			playlists.c
libupod_la_LDFLAGS = -version-info 0:0:1
libupod_la_LIBADD =