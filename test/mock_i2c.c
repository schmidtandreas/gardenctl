/*
 * mock_i2c.c
 * This file is a part of gardenctl
 *
 * Copyright (C) 2018 Andreas Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <linux/i2c-dev.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "mock_i2c.h"

#include "garden_common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
static int check_pathname(const LargestIntegralType value, const LargestIntegralType data)
{
	int ret;
	const char *pathname = (const char*)value;
	const char *pattern = (const char*)data;

	ret = match_regex(pattern, pathname);
	if (!ret)
		fprintf(stderr, "%s not match regex %s\n", pathname, pattern);

	return ret;
}
#pragma GCC diagnostic pop

int __wrap_open(const char *pathname, int flags)
{
	int fd = mock_type(int);

	check_expected(pathname);

	return fd;
}

int __wrap_ioctl(int fd, unsigned long request, ...)
{
	va_list vl;

	check_expected(request);

	va_start(vl, request);
	uint8_t addr = (uint8_t)va_arg(vl, int);
	va_end(vl);

	assert_in_range(addr, 0x3, 0x77);

	return mock_type(int);
}

ssize_t __wrap_read(int fd, void *buf, size_t count)
{
	void *data = mock_type(void*);

	check_expected(count);
	memcpy(buf, data, count);
	return mock_type(int);
}

ssize_t __wrap_write(int fd, const void *buf, size_t count)
{
	check_expected(count);
	check_expected_ptr(buf);

	return mock_type(int);
}

int __wrap_close(int fd)
{
	return 0;
}

void mock_i2c_prepare_write(int ret_open, int ret_ioctl, const char *wbuf,
			    size_t wbuf_size, int ret_write)
{
	will_return(__wrap_open, ret_open);
	expect_check(__wrap_open, pathname, check_pathname,
		     strdup("/dev/i2c-[0-1]"));
	if (ret_open < 0)
		return;

	expect_value(__wrap_ioctl, request, I2C_SLAVE);
	will_return(__wrap_ioctl, ret_ioctl);
	if (ret_ioctl < 0)
		return;

	expect_value(__wrap_write, count, 1);
	expect_memory(__wrap_write, buf, wbuf, wbuf_size);
	will_return(__wrap_write, ret_write);
}

void mock_i2c_prepare_read(int ret_open, int ret_ioctl, const char *rbuf,
			   int ret_read)
{
	will_return(__wrap_open, ret_open);
	expect_check(__wrap_open, pathname, check_pathname,
		     strdup("/dev/i2c-[0-1]"));
	if (ret_open < 0)
		return;

	expect_value(__wrap_ioctl, request, I2C_SLAVE);
	will_return(__wrap_ioctl, ret_ioctl);
	if (ret_ioctl < 0)
		return;

	expect_value(__wrap_read, count, 1);
	will_return(__wrap_read, rbuf);
	will_return(__wrap_read, ret_read);
}
