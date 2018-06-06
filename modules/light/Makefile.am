moduleslibdir = $(MODULESDIR)

moduleslib_LTLIBRARIES = libgarden_light.la

libgarden_light_la_SOURCES = light.c

libgarden_light_la_LIBADD = $(top_builddir)/common/libgarden_common.la

libgarden_light_la_CFLAGS = -I$(top_srcdir)/include \
	-I$(top_srcdir)/common/include

libgarden_light_la_LDFLAGS = -no-undefined -avoid-version -shared -module
