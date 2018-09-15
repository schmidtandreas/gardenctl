/*
 * mock_dl.c
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

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "mock_dl.h"

void *__wrap_dlopen(const char *filename, int flag)
{
	check_expected_ptr(filename);
	return mock_ptr_type(void *);
}

char *__wrap_dlerror(void)
{
	return mock_ptr_type(char *);
}

void *__wrap_dlsym(void *handle, const char *symbol)
{
	check_expected_ptr(handle);
	check_expected_ptr(symbol);
	return mock_ptr_type(void *);
}

int __wrap_dlclose(void *handle)
{
	check_expected_ptr(handle);
	return mock_type(int);
}

void mock_dlopen(const char *dl_filename, void *dl_handle, const char *dl_error)
{
	expect_string(__wrap_dlopen, filename, dl_filename);
	will_return(__wrap_dlopen, dl_handle);
	will_return(__wrap_dlerror, dl_error);
}

void mock_dlsym(void *dl_handle, const char *symname, void *ret,
		const char *dl_error)
{
	expect_value(__wrap_dlsym, handle, dl_handle);
	expect_string(__wrap_dlsym, symbol, symname);
	will_return(__wrap_dlsym, ret);
	will_return(__wrap_dlerror, dl_error);
}

void mock_dlclose(void *dl_handle, int ret)
{
	expect_value(__wrap_dlclose, handle, dl_handle);
	will_return(__wrap_dlclose, ret);
}
