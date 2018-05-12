moduleslibdir = $(MODULESDIR)

moduleslib_LTLIBRARIES = libgarden_watering.la

libgarden_watering_la_SOURCES = watering.c

libgarden_watering_la_LIBADD = $(top_builddir)/common/libgarden_common.la

libgarden_watering_la_CFLAGS = -I$(top_srcdir)/include \
	-I$(top_srcdir)/common/include

libgarden_watering_la_LDFLAGS = -no-undefined -avoid-version -shared -module
