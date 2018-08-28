TESTS = check_gardenctl
check_PROGRAMS = check_gardenctl

check_gardenctl_SOURCES = check_gardenctl.c

check_gardenctl_CFLAGS = @CHECK_CFLAGS@ -I$(top_srcdir)/common/include

check_gardenctl_LDADD = @CHECK_LIBS@ $(top_builddir)/common/libgarden_common.la
