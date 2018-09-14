AM_CFLAGS = -DDEBUG -std=c99 -O2 -Wall -Werror -D_GNU_SOURCE -DDEFAULT_MODULE_PATH=\"$(MODULESDIR)/\" -DDEFAULT_CONF_PATH=\"$(CONFPATH)\"

bin_PROGRAMS = gardenctl

gardenctl_SOURCES = gardenctl.c mqtt.c dl_module.c

gardenctl_CFLAGS = ${AM_CFLAGS} \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common/include

gardenctl_LDFLAGS =  ${AM_LDFLAGS} -ldl -lmosquitto -lconfuse

gardenctl_LDADD = $(top_builddir)/common/libgarden_common.la

noinst_HEADERS = mqtt.h dl_module.h arguments.h $(top_srcdir)/include/garden_module.h
