AC_DEFUN([AC_WITH_SYSTEMD], [
	AC_ARG_WITH([systemdsystemunitdir],
		AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [path to systemd system units @<:@default=/lib/systemd/system@:>@]),
		[SYSTEMDSYSTEMUNITDIR=${withval}],
		[SYSTEMDSYSTEMUNITDIR=/lib/systemd/system])

	AC_SUBST(SYSTEMDSYSTEMUNITDIR)
])

AC_DEFUN([AC_WITH_PID], [
	AC_ARG_WITH([pid],
		AS_HELP_STRING([--with-pid=FILE], [path to location for PID file @<:@default=/var/run/gardenctl.pid@:>@]),
		[PIDFILE=${withval}],
		[PIDFILE=/var/run/gardenctl.pid])

	AC_SUBST(PIDFILE)
	AC_DEFINE_UNQUOTED([PIDFILE], ["${PIDFILE}"], ["path to location for PID file"])
])

AC_DEFUN([AC_WITH_MODDIR], [
	AC_ARG_WITH([moddir],
		AS_HELP_STRING([--with-moddir=DIR], [path to location of gardenctl modules @<:@default=$libdir/gardenctl@:>@]),
		[MODULESDIR=${withval}],
		[MODULESDIR=$libdir/gardenctl])

	AC_SUBST(MODULESDIR)
])

AC_DEFUN([AC_WITH_CONFPATH], [
	AC_ARG_WITH([conf],
		AS_HELP_STRING([--with-conf=FILE], [path to location of gardenctl configuration file @<:@default=/etc/default/gardenctl@:>@]),
		[CONFPATH=${withval}],
		[CONFPATH=/etc/default/gardenctl])

	AC_SUBST(CONFPATH)
])


