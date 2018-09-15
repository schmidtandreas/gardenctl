/*
 * check_garden_common.c
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

#include "mock_malloc.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

void *__real_malloc(size_t size);
void *__real_calloc(size_t nmemb, size_t size);
void __real_free(void *ptr);

void *__wrap_malloc(size_t size)
{
	check_expected(size);
	return mock_ptr_type(void *);
}

void *__wrap_calloc(size_t nmemb, size_t size)
{
	check_expected(size);
	return mock_ptr_type(void *);
}

void __wrap_free(void *ptr)
{
	check_expected_ptr(ptr);
	__real_free(ptr);
}

void mock_calloc(size_t exp_size, size_t alloc_size)
{
	void *pmem = NULL;

	if (alloc_size)
		pmem = __real_calloc(1, alloc_size);

	expect_value(__wrap_calloc, size, exp_size);
	will_return(__wrap_calloc, pmem);

	if (pmem)
		expect_value(__wrap_free, ptr, pmem);
}

