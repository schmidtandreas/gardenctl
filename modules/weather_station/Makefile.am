moduleslibdir = $(MODULESDIR)

moduleslib_LTLIBRARIES = libgarden_weather_station.la

libgarden_weather_station_la_SOURCES = weather_station.c

libgarden_weather_station_la_LIBADD = $(top_builddir)/common/libgarden_common.la

libgarden_weather_station_la_CFLAGS = -I$(top_srcdir)/include \
	-I$(top_srcdir)/common/include

libgarden_weather_station_la_LDFLAGS = -lpthread -no-undefined -avoid-version -shared -module
