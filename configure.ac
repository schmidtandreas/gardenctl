#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gardenctl], m4_esyscmd(build-aux/git-version-gen .tarball-version), [mail@schmidt-andreas.de])
AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_SRCDIR([gardenctl/gardenctl.c])
AC_CONFIG_HEADERS([include/config.h])
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz color-tests parallel-tests subdir-objects foreign -Wall -Werror])
AC_CONFIG_MACRO_DIR([m4])
#AC_PREFIX_DEFAULT([/usr])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_AR

# Initiate libtool
LT_INIT([dlopen])

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([mosquitto], [mosquitto_lib_version])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h signal.h stdarg.h dirent.h regex.h sys/queue.h \
		  dlfcn.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)

# Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([ \
		daemon strerror fprintf vfprintf getpid \
		opendir readdir closedir regcomp \
		dlerror dlclose dlsym \
])

# Additional arguments
AC_ARG_WITH([pid],
	AS_HELP_STRING([--with-pid=FILE], [path to location for PID file @<:@default=/var/run/gardenctl.pid@:>@]),
	[with_pid=${withval}],
	[with_pid=/var/run/gardenctl.pid])
if (test -n "${with_pid}" ); then
	AC_DEFINE_UNQUOTED([PIDFILE], ["${with_pid}"], ["path to location for PID file"])
fi

AC_ARG_ENABLE([moddir],
	AS_HELP_STRING([--enable-moddir=DIR], [path to location of gardenctl modules @<:@default=$libdir/gardenctl@:>@]),
	[MODULESDIR=${enableval}],
	[MODULESDIR=$libdir/gardenctl])

AC_SUBST(MODULESDIR)

AC_CONFIG_FILES([
	Makefile
	common/Makefile
	modules/Makefile
	modules/watering/Makefile
	gardenctl/Makefile
])
AC_OUTPUT
