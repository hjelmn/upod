OBJS = db.o tihm.o endian.o dohm.o mp3.o mp3tech.o unicode.o pihm.o non-posix.o
SRCS = db.c tihm.c endian.c dohm.c mp3.c mp3tech.c unicode.c pihm.c non-posix.c

CFLAGS = -g3 -I../include -traditional-cpp -D__darwin__
GCC = gcc
.c.o: ../include/upodi.h
	$(GCC) -c $(CFLAGS) $*.c

all: db_add db_remove db_hide db_unhide db_list

db_add: $(OBJS) add.o
	$(GCC) -o db_add -g3 $(OBJS) add.o

db_remove: $(OBJS) remove.o
	$(GCC) -o db_remove -g3 $(OBJS) remove.o

db_hide: $(OBJS) hide.o
	$(GCC) -o db_hide -g3 $(OBJS) hide.o

db_unhide: $(OBJS) unhide.o
	$(GCC) -o db_unhide -g3 $(OBJS) unhide.o

db_list: $(OBJS) list.o
	$(GCC) -o db_list -g3 $(OBJS) list.o

clean:
	rm -f $(OBJS)

clean_all:
	rm -f $(OBJS) db_add db_remove db_hide db_unhide db_list add.o remove.o hide.o unhide.o list.o

