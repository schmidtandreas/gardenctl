## Copyright (C) 2018 by Andreas Schmidt

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_CFLAGS = -std=c99 -O2 -Wall -Werror -D_GNU_SOURCE

SUBDIRS = common gardenctl modules

EXTRA_DIST = \
	$(top_srcdir)/.version \
	autogen.sh
	configure

BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
