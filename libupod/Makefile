OBJS =  db.o tihm.o endian.o dohm.o mp3.o mp3tech.o unicode.o pihm.o non-posix.o \
	playlist.o pyhm.o create.o dshm.o checks.o ipod.o log.o
SRCS =  db.c tihm.c endian.c dohm.c mp3.c mp3tech.c unicode.c pihm.c non-posix.c \
	ipod.c playlist.c pyhm.c create.c dshm.c checks.c ipod.c log.c

CFLAGS = -g3 -I../include -traditional-cpp -D__darwin__ -ansi
LDFLAGS= -lMallocDebug
GCC = gcc
.c.o: ../include/upodi.h ../include/upod.h
	$(GCC) -c $(CFLAGS) $*.c

all: libipod.a db_add db_remove db_hide db_unhide db_list db_adddir db_pl

libipod.a: $(OBJS)
	ar cr libipod.a $(OBJS)
	ranlib libipod.a

db_add: libipod.a add.o
	$(GCC) $(LDFLAGS) -o db_add -g3 -L. $(OBJS) add.o

db_remove: libipod.a remove.o
	$(GCC) $(LDFLAGS) -o db_remove -g3 -L. $(OBJS) remove.o

db_hide: libipod.a hide.o
	$(GCC) $(LDFLAGS) -o db_hide -g3 -L. $(OBJS) hide.o

db_unhide: libipod.a unhide.o
	$(GCC) $(LDFLAGS) -o db_unhide -g3 -L. $(OBJS) unhide.o

db_list: libipod.a list.o
	$(GCC) $(LDFLAGS) -o db_list -g3 -L. $(OBJS) list.o

db_adddir: libipod.a add_dir.o
	$(GCC) $(LDFLAGS) -o db_adddir -g3 -L. $(OBJS) add_dir.o

db_pl: libipod.a pl.o
	$(GCC) $(LDFLAGS) -o db_pl -g3 -L. $(OBJS) pl.o

clean:
	rm -f $(OBJS)

clean_all:
	rm -f $(OBJS) libipod.a db_adddir db_add db_remove db_hide db_unhide db_list db_lookup db_pl add_dir.o pl.o add.o remove.o hide.o unhide.o list.o lookup.o

