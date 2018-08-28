/*
 * check_gardenctl.c
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
#include <errno.h>
#include <check.h>
#include "garden_common.h"
#include "logging.h"

START_TEST(test_payload2int){
	const char *payload[8];

	payload[0] = "on";
	payload[1] = "off";
	payload[2] = "o";
	payload[3] = "of";
	payload[4] = "";
	payload[5] = "big";
	payload[6] = "bigger";
	payload[7] = "l";

	ck_assert_int_eq(payload_on_off_to_int(payload[0]), 1);
	ck_assert_int_eq(payload_on_off_to_int(payload[1]), 0);
	ck_assert_int_eq(payload_on_off_to_int(payload[2]), -EINVAL);
	ck_assert_int_eq(payload_on_off_to_int(payload[3]), -EINVAL);
	ck_assert_int_eq(payload_on_off_to_int(payload[4]), -EINVAL);
	ck_assert_int_eq(payload_on_off_to_int(payload[5]), -EINVAL);
	ck_assert_int_eq(payload_on_off_to_int(payload[6]), -EINVAL);
	ck_assert_int_eq(payload_on_off_to_int(payload[7]), -EINVAL);
	ck_assert_int_eq(payload_on_off_to_int(NULL), -EINVAL);
}
END_TEST

START_TEST(test_logging)
{
	ck_assert_int_ge(max_loglevel, LOGLEVEL_INFO);
	ck_assert_int_le(max_loglevel, LOGLEVEL_DEBUG);

	ck_assert_int_eq(logging(  -1, ""), 0);
	ck_assert_int_eq(logging(   4, ""), 0);
	ck_assert_int_eq(logging(3203, ""), 0);

	/*TODO: check max fmt size limit*/
}
END_TEST

Suite * modules_suite(void)
{
	Suite *s;
	TCase *tc_common;

	s = suite_create("GardenCtl");

	tc_common = tcase_create("Common");

	tcase_add_test(tc_common, test_payload2int);
	tcase_add_test(tc_common, test_logging);

	suite_add_tcase(s, tc_common);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = modules_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
