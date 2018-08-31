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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "mock_i2c.h"

#include "garden_common.h"
#include "logging.h"
#include "gpioex.h"

static void test_payload2int(void **state)
{
	const char *payload[8];

	payload[0] = "on";
	payload[1] = "off";
	payload[2] = "o";
	payload[3] = "of";
	payload[4] = "";
	payload[5] = "big";
	payload[6] = "bigger";
	payload[7] = "l";

	assert_int_equal(payload_on_off_to_int(payload[0]), 1);
	assert_int_equal(payload_on_off_to_int(payload[1]), 0);
	assert_int_equal(payload_on_off_to_int(payload[2]), -EINVAL);
	assert_int_equal(payload_on_off_to_int(payload[3]), -EINVAL);
	assert_int_equal(payload_on_off_to_int(payload[4]), -EINVAL);
	assert_int_equal(payload_on_off_to_int(payload[5]), -EINVAL);
	assert_int_equal(payload_on_off_to_int(payload[6]), -EINVAL);
	assert_int_equal(payload_on_off_to_int(payload[7]), -EINVAL);
	assert_int_equal(payload_on_off_to_int(NULL), -EINVAL);
}

static void test_logging(void **state)
{
	assert_true(max_loglevel >= LOGLEVEL_INFO);
	assert_true(max_loglevel <= LOGLEVEL_DEBUG);

	assert_int_equal(logging(  -1, ""), 0);
	assert_int_equal(logging(   4, ""), 0);
	assert_int_equal(logging(3203, ""), 0);

	/*TODO: check max fmt size limit*/
}

static void test_gpioex_init(void **state)
{
	uint8_t val = 0xFF;
	int fd;

	for (fd = 3; fd < 6; ++fd)
		mock_i2c_prepare_write(fd, 0, &val, sizeof(val), sizeof(val));
	assert_null(gpioex_init());

	/* Check open failed */
	mock_i2c_prepare_write(-ENODEV, 0, NULL, 0, 0);
	assert_int_equal(gpioex_init(), -ENODEV);

	/* Check ioctl failed */
	mock_i2c_prepare_write(3, -EINVAL, NULL, 0, 0);
	assert_int_equal(gpioex_init(), -EINVAL);

	/* Check write 0 */
	mock_i2c_prepare_write(3, 0, &val, sizeof(val), 0);
	assert_int_equal(gpioex_init(), -EIO);

	/* Check write failed */
	mock_i2c_prepare_write(3, 0, &val, sizeof(val), -ENOSPC);
	assert_int_equal(gpioex_init(), -ENOSPC);
}

void test_gpioex_set(void **state)
{
	uint8_t valves_value = 0xFF;
	uint8_t result_valves_val = 0xF7;
	uint8_t val230v = 0xFF;
	uint8_t result_230v_val = 0x7F;

	mock_i2c_prepare_read(3, 0, &valves_value, 1);
	mock_i2c_prepare_write(3, 0, &result_valves_val, 1, 1);
	mock_i2c_prepare_read(3, 0, &val230v, 1);
	mock_i2c_prepare_write(3, 0, &result_230v_val, 1, 1);
	assert_null(gpioex_set(GPIOEX_TAP, 1));
}

static int is_valid_barrel_lvl(uint8_t val)
{
	if (val == 0xFF || val == 0xFE ||
	    val == 0xFC || val == 0xF8 ||
	    val == 0xF0 || val == 0xE0 ||
	    val == 0xC0 || val == 0x80 ||
	    val == 0x00)
		return 1;
	else
		return 0;
}

static void test_gpioex_get_barrel_level(void **state)
{
	char barrel_lvl = 0xFF;
	int i;

	for (i = 0; i <= 16; ++i) {
		mock_i2c_prepare_read(3, 0, &barrel_lvl, 1);
		assert_int_equal(gpioex_get_barrel_level(), (unsigned char)barrel_lvl);

		if (i == 0)
			barrel_lvl = 0xFE;
		else if (i < 8)
			barrel_lvl <<= 1;
		else
			barrel_lvl = (barrel_lvl >> 1) | 0x80;
	}

	for (i = 0; i <= 255; ++i) {
		mock_i2c_prepare_read(3, 0, (char*)&i, 1);
		if (is_valid_barrel_lvl((uint8_t)i))
			assert_int_equal(gpioex_get_barrel_level(), i);
		else
			assert_int_equal(gpioex_get_barrel_level(), -EINVAL);
	}

	/* Check open failed */
	mock_i2c_prepare_read(-ENODEV, 0, NULL, 0);
	assert_int_equal(gpioex_get_barrel_level(), -ENODEV);

	/* Check ioctl failed */
	mock_i2c_prepare_read(3, -EINVAL, NULL, 0);
	assert_int_equal(gpioex_get_barrel_level(), -EINVAL);

	/* Check read 0 */
	mock_i2c_prepare_read(3, 0, &barrel_lvl, 0);
	assert_int_equal(gpioex_get_barrel_level(), -EIO);

	/* Check read failed */
	mock_i2c_prepare_read(3, 0, &barrel_lvl, -EINVAL);
	assert_int_equal(gpioex_get_barrel_level(), -EINVAL);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_payload2int),
		cmocka_unit_test(test_logging),
		cmocka_unit_test(test_gpioex_init),
		cmocka_unit_test(test_gpioex_set),
		cmocka_unit_test(test_gpioex_get_barrel_level),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
