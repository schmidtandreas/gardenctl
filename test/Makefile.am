TESTS = check_garden_common
check_PROGRAMS = check_garden_common

check_garden_common_SOURCES = mock_i2c.c mock_i2c.h check_garden_common.c

check_garden_common_CFLAGS = @CMOCKA_CFLAGS@ -I$(top_srcdir)/common/include

check_garden_common_LDFLAGS = -Wl,--wrap=open -Wl,--wrap=ioctl -Wl,--wrap=write -Wl,--wrap=read -Wl,--wrap=close

check_garden_common_LDADD = @CMOCKA_LIBS@ $(top_builddir)/common/libgarden_common.la
