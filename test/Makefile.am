check_PROGRAMS = check_garden_common check_gardenctl_dl_module

check_garden_common_SOURCES = mock_i2c.c mock_i2c.h check_garden_common.c

check_garden_common_CFLAGS = @CMOCKA_CFLAGS@ -I$(top_srcdir)/common/include

check_garden_common_LDFLAGS = -Wl,--wrap=open -Wl,--wrap=ioctl -Wl,--wrap=write -Wl,--wrap=read -Wl,--wrap=close

check_garden_common_LDADD = @CMOCKA_LIBS@ $(top_builddir)/common/libgarden_common.la


check_gardenctl_dl_module_SOURCES = mock_malloc.c mock_malloc.h mock_dl.c mock_dl.h ../gardenctl/dl_module.c check_gardenctl_dl_module.c

check_gardenctl_dl_module_CFLAGS = @CMOCKA_CFLAGS@ -I$(top_srcdir)/include -I$(top_srcdir)/gardenctl -I$(top_srcdir)/common/include

check_gardenctl_dl_module_LDFLAGS = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=dlerror, -Wl,--wrap=dlopen -Wl,--wrap=dlsym -Wl,--wrap=dlclose -Wl,--wrap=free

check_gardenctl_dl_module_LDADD = @CMOCKA_LIBS@ $(top_builddir)/common/libgarden_common.la

TESTS = $(check_PROGRAMS)
