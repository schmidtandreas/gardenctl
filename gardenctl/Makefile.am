AM_CFLAGS = -DDEBUG -std=c99 -O2 -Wall -Werror -D_GNU_SOURCE -DDEFAULT_MODULE_PATH=\"$(MODULESDIR)/\"

bin_PROGRAMS = gardenctl

gardenctl_SOURCES = gardenctl.c

gardenctl_CFLAGS = ${AM_CFLAGS} \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common/include

gardenctl_LDFLAGS =  ${AM_LDFLAGS} -ldl

gardenctl_LDADD = $(top_builddir)/common/libgarden_common.la

noinst_HEADERS = arguments.h
