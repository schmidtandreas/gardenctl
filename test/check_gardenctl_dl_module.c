/*
 * check_gardenctl_dl_module.c
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
#include <errno.h>
#include <string.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <dl_module.h>
#include <logging.h>

#include "mock_malloc.h"
#include "mock_dl.h"

static struct garden_module* mock_create_garden_module(enum loglevel maxloglvl)
{
	return mock_ptr_type(struct garden_module*);
}

static void mock_destroy_garden_module(struct garden_module* module)
{

}

static void test_dl_create(void **state)
{
	dlm_head_t dlm_head;
	void *dl_handle;
	struct garden_module gm;

	LIST_INIT(&dlm_head);

	mock_calloc(sizeof(dlm_t), 0);
	assert_int_equal(dlm_create(NULL, NULL), -ENOMEM);

	mock_calloc(sizeof(dlm_t), sizeof(dlm_t));
	assert_int_equal(dlm_create(NULL, NULL), -EINVAL);

	mock_calloc(sizeof(dlm_t), sizeof(dlm_t));
	assert_int_equal(dlm_create(NULL, "module"), -EINVAL);

	mock_calloc(sizeof(dlm_t), sizeof(dlm_t));
	assert_int_equal(dlm_create(&dlm_head, NULL), -EINVAL);

	mock_calloc(sizeof(dlm_t), sizeof(dlm_t));
	will_return(__wrap_dlerror, NULL);
	mock_dlopen("not_exists", NULL, "module not exists");
	assert_int_equal(dlm_create(&dlm_head, "not_exists"), -EINVAL);

	dl_handle = (void*)1;
	mock_calloc(sizeof(dlm_t), sizeof(dlm_t));
	will_return(__wrap_dlerror, NULL);
	mock_dlopen("invalid_module", dl_handle, NULL);
	mock_dlsym(dl_handle, "create_garden_module", NULL,
		   "No symbol create_garden_module found");
	mock_dlclose(dl_handle, 0);
	assert_int_equal(dlm_create(&dlm_head, "invalid_module"), 1);

	dl_handle = (void*)1;
	mock_calloc(sizeof(dlm_t), sizeof(dlm_t));
	will_return(__wrap_dlerror, NULL);
	mock_dlopen("invalid_module", dl_handle, NULL);
	mock_dlsym(dl_handle, "create_garden_module", mock_create_garden_module,
		   NULL);
	mock_dlsym(dl_handle, "destroy_garden_module", NULL,
		   "No symbol destroy_garden_module found");
	mock_dlclose(dl_handle, 0);
	assert_int_equal(dlm_create(&dlm_head, "invalid_module"), 1);

	dl_handle = (void*)1;
	mock_calloc(sizeof(dlm_t), sizeof(dlm_t));
	will_return(__wrap_dlerror, NULL);
	mock_dlopen("module", dl_handle, NULL);
	mock_dlsym(dl_handle, "create_garden_module", mock_create_garden_module,
		   NULL);
	mock_dlsym(dl_handle, "destroy_garden_module",
		   mock_destroy_garden_module, NULL);
	will_return(mock_create_garden_module, NULL);
	mock_dlclose(dl_handle, 0);
	assert_int_equal(dlm_create(&dlm_head, "module"), -ENOMEM);

	dl_handle = (void*)1;
	mock_calloc(sizeof(dlm_t), sizeof(dlm_t));
	will_return(__wrap_dlerror, NULL);
	mock_dlopen("module", dl_handle, NULL);
	mock_dlsym(dl_handle, "create_garden_module", mock_create_garden_module,
		   NULL);
	mock_dlsym(dl_handle, "destroy_garden_module",
		   mock_destroy_garden_module, NULL);
	will_return(mock_create_garden_module, &gm);
	assert_int_equal(dlm_create(&dlm_head, "module"), 0);
	free(dlm_head.lh_first);
}

static void test_dl_destroy(void **state)
{
}

static void test_dl_mod_init(void **state)
{
}

static void test_dl_mod_subscribe(void **state)
{
}

static void test_dl_mod_message(void **state)
{
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_dl_create),
		cmocka_unit_test(test_dl_destroy),
		cmocka_unit_test(test_dl_mod_init),
		cmocka_unit_test(test_dl_mod_subscribe),
		cmocka_unit_test(test_dl_mod_message),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
