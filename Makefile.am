bin_PROGRAMS=upod
upod_SOURCES=upod.c xmalloc.c system.h aclocal.m4 checkopt.c

#  add " -lm" to the following if upod requires the math library
upod_LDADD=-lopts

EXTRA_DIST=upod.lsm.in upod.spec.in checkopt.def upod.1 manpage.tpl

#  if you write a self-test script named `chk', uncomment the
#  following and add `chk' to the EXTRA_DIST list
#TESTS=chk

upod.o: checkopt.h
checkopt.o: checkopt.c
checkopt.c checkopt.h: $(srcdir)/checkopt.def
	autogen -L/usr/share/autogen $(srcdir)/checkopt.def

upod.1: $(srcdir)/checkopt.def
	autogen -T manpage.tpl  $(srcdir)/checkopt.def
	awk -f updater upod.1 > tmp$$$$ && mv tmp$$$$ upod.1
#	rm updater

# info_TEXINFOS = upod.texinfo
# upod_TEXINFOS = gpl.texinfo

#  install the man page
man_MANS=upod.1
