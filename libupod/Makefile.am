libdir = $(prefix)/lib

INCLUDES += -I$(top_srcdir)/include
CFLAGS += -ansi `pkg-config --cflags glib` -g3
LDFLAGS += `pkg-config --libs glib`

lib_LTLIBRARIES = libupod.la
libupod_la_SOURCES = db.c tihm.c endian.c dohm.c aac.c mp3.c mp3tech.c unicode.c pihm.c \
	non-posix.c playlist.c pyhm.c create.c dshm.c checks.c ipod.c log.c lookup.c
libupod_la_LDFLAGS = -version-info 3:1:0
