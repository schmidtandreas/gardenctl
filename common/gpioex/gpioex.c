/*
 * gpioex.c
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

#include <gpioex.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <logging.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <errno.h>

static pthread_mutex_t lock;

#define GPIOEX_BUS_1			0x01
#define GPIOEX_ADDR_BARREL_LVL		0x20
#define GPIOEX_ADDR_230V		0x21
#define GPIOEX_ADDR_24V			0x22
#define GPIOEX_ADDR_12V			0x23

/* 24V relays */
#define GPIOEX_BIT_TAP			0x08
#define GPIOEX_BIT_BARREL		0x04
#define GPIOEX_BIT_YARD_LEFT		0x10
#define GPIOEX_BIT_YARD_RIGHT		0x20
#define GPIOEX_BIT_YARD_FRONT		0x40
#define GPIOEX_BIT_YARD_BACK		0x80

#define GPIOEX_BIT_YARD	(GPIOEX_BIT_YARD_LEFT | GPIOEX_BIT_YARD_RIGHT | \
			 GPIOEX_BIT_YARD_FRONT | GPIOEX_BIT_YARD_BACK)

#define GPIOEX_BIT_PUMP_VALVES	(GPIOEX_BIT_YARD | GPIOEX_BIT_TAP | \
				 GPIOEX_BIT_BARREL)

/* 230V relays */
#define GPIOEX_BIT_PUMP			0x80
#define GPIOEX_BIT_LIGHT_TREE		0x08
#define GPIOEX_BIT_LIGHT_HOUSE		0x04

/* 12V relays */
#define GPIOEX_BIT_DROPPIPE_PUMP	0x80
#define GPIOEX_BIT_LIGHT_TAP		0x20

static int gpioex_set_gpio(uint8_t bus, uint8_t addr, uint8_t value)
{
	int ret = 0;
	int fd;
	char filename[20];

	memset(filename, 0, sizeof(filename));

	sprintf(filename, "/dev/i2c-%d", bus);

	ret = open(filename, O_RDWR);
	if (ret < 0) {
		log_err("open %s failed (%d) %s", filename, ret, strerror(ret));
		goto out;
	}

	fd = ret;

	ret = ioctl(fd, I2C_SLAVE, addr);
	if (ret < 0) {
		log_err("set address to %s failed (%d) %s", filename, ret,
			strerror(ret));
		goto out_close;
	}

	ret = write(fd, &value, 1);
	if (ret < 0) {
		log_err("write to %s failed (%d) %s", filename, ret, strerror(ret));
		goto out_close;
	}

	log_dbg("i2c write: addr: %d val: %02X", addr, value);
	ret = 0;

out_close:
	close(fd);
out:
	return ret;
}

static int gpioex_get_gpio(uint8_t bus, uint8_t addr, uint8_t *value)
{
	int ret = 0;
	int fd;
	char filename[20];

	if (!value) {
		log_err("value points to NULL");
		ret = -EINVAL;
		goto out;
	}

	memset(filename, 0, sizeof(filename));
	sprintf(filename, "/dev/i2c-%d", bus);

	ret = open(filename, O_RDWR);
	if (ret < 0) {
		log_err("open %s failed (%d) %s", filename, ret, strerror(ret));
		goto out;
	}

	fd = ret;

	ret = ioctl(fd, I2C_SLAVE, addr);
	if (ret < 0) {
		log_err("set address to %s failed (%d) %s", filename, ret,
			strerror(ret));
		goto out_close;
	}

	ret = read(fd, value, 1);
	if (ret != 1) {
		log_err("read from %s failed (%d) %s", filename, ret, strerror(ret));
		goto out_close;
	}

	log_dbg("i2c read addr: %d: %02X", addr, *value);

	ret = 0;

out_close:
	close(fd);
out:
	return ret;
}

static int gpioex_disable_all(void)
{
	int ret = 0;

	ret = gpioex_set_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_230V, 0xFF);
	if (ret)
		goto out;

	ret = gpioex_set_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_24V, 0xFF);
	if (ret)
		goto out;

	ret = gpioex_set_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_12V, 0xFF);
	if (ret)
		goto out;

	return 0;
out:
	return ret;
}

int gpioex_init(void)
{
	int ret = 0;

	ret = pthread_mutex_init(&lock, NULL);
	if (ret) {
		log_err("initiate mutex failed (%d) %s", ret, strerror(ret));
		goto out;
	}

	ret = gpioex_disable_all();
	if (ret)
		goto out;

	return 0;
out:
	return ret;
}

static int gpioex_set_valves_and_pump(uint8_t valves_value)
{
	int ret = 0;

	log_dbg("set valves %02X", valves_value);

	if ((valves_value & GPIOEX_BIT_PUMP_VALVES) == GPIOEX_BIT_PUMP_VALVES) {
		uint8_t val230v;

		ret = gpioex_get_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_230V, &val230v);
		if (ret)
			goto out;

		val230v |= GPIOEX_BIT_PUMP;

		ret = gpioex_set_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_230V, val230v);
		if (ret)
			goto out;

		log_dbg("pump disabled");

		ret = gpioex_set_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_24V,
				      valves_value);
		if (ret)
			goto out;
	} else {
		uint8_t val230v;

		ret = gpioex_set_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_24V,
				      valves_value);
		if (ret)
			goto out;

		ret = gpioex_get_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_230V, &val230v);
		if (ret)
			goto out;

		val230v &= ~GPIOEX_BIT_PUMP;

		ret = gpioex_set_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_230V, val230v);
		if (ret)
			goto out;
		log_dbg("pump enabled");
	}

out:
	return ret;
}

#define GPIOEX_SET_BIT(__addr, __bit, __val, __ret, __out) do { \
		uint8_t val; \
		__ret = gpioex_get_gpio(GPIOEX_BUS_1, __addr, &val); \
		if (__ret) \
			goto __out; \
		if (!__val) \
			val |= GPIOEX_BIT_ ## __bit; \
		else \
			val &= ~GPIOEX_BIT_ ## __bit; \
		__ret = gpioex_set_gpio(GPIOEX_BUS_1, __addr, val); \
		if (__ret) \
			goto __out; \
} while (0)

int gpioex_set(uint32_t gpio, int value)
{
	int ret = 0;
	int set_valves = 0;
	uint8_t valves_value;

	log_dbg("gpioex set: gpio: %08X val: %d", gpio, value);

	pthread_mutex_lock(&lock);

	ret = gpioex_get_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_24V, &valves_value);
	if (ret < 0)
		goto out;

	if (gpio & GPIOEX_TAP) {
		if (!value)
			valves_value |= GPIOEX_BIT_TAP;
		else
			valves_value &= ~GPIOEX_BIT_TAP;
		set_valves = 1;
	}

	if (gpio & GPIOEX_BARREL) {
		if (!value)
			valves_value |= GPIOEX_BIT_BARREL;
		else
			valves_value &= ~GPIOEX_BIT_BARREL;
		set_valves = 1;
	}

	if (gpio & GPIOEX_YARD) {
		valves_value |= GPIOEX_BIT_YARD;
		if (value) {
			if (gpio & GPIOEX_YARD_LEFT)
				valves_value &= ~GPIOEX_BIT_YARD_LEFT;
			if (gpio & GPIOEX_YARD_RIGHT)
				valves_value &= ~GPIOEX_BIT_YARD_RIGHT;
			if (gpio & GPIOEX_YARD_FRONT)
				valves_value &= ~GPIOEX_BIT_YARD_FRONT;
			if (gpio & GPIOEX_YARD_BACK)
				valves_value &= ~GPIOEX_BIT_YARD_BACK;
		}

		set_valves = 1;
	}

	if (set_valves) {
		ret = gpioex_set_valves_and_pump(valves_value);
		if (ret)
			goto out;
	}

	if (gpio & GPIOEX_DROPPIPE_PUMP)
		GPIOEX_SET_BIT(GPIOEX_ADDR_12V, DROPPIPE_PUMP, value, ret, out);

	if (gpio & GPIOEX_LIGHT_TREE)
		GPIOEX_SET_BIT(GPIOEX_ADDR_230V, LIGHT_TREE, value, ret, out);

	if (gpio & GPIOEX_LIGHT_HOUSE)
		GPIOEX_SET_BIT(GPIOEX_ADDR_230V, LIGHT_HOUSE, value, ret, out);

	if (gpio & GPIOEX_LIGHT_TAP)
		GPIOEX_SET_BIT(GPIOEX_ADDR_12V, LIGHT_TAP, value, ret, out);
out:
	pthread_mutex_unlock(&lock);
	return ret;
}

int gpioex_get_barrel_level(void)
{
	int ret = 0;
	uint8_t lvl;

	pthread_mutex_lock(&lock);

	ret = gpioex_get_gpio(GPIOEX_BUS_1, GPIOEX_ADDR_BARREL_LVL, &lvl);
	if (!ret)
		ret = lvl;

	pthread_mutex_unlock(&lock);

	return ret;
}
