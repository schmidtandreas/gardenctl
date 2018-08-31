/*
 * gpioex.h
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

#ifndef __GPIOEX_H__
#define __GPIOEX_H__

#include <stdint.h>

#define GPIOEX_TAP		0x00000001
#define GPIOEX_YARD_LEFT	0x00000002
#define GPIOEX_YARD_RIGHT	0x00000004
#define GPIOEX_YARD_FRONT	0x00000008
#define GPIOEX_YARD_BACK	0x00000010
#define GPIOEX_DROPPIPE_PUMP	0x00000020
#define GPIOEX_BARREL		0x00000040
#define GPIOEX_LIGHT_TREE	0x00000080
#define GPIOEX_LIGHT_HOUSE	0x00000100
#define GPIOEX_LIGHT_TAP	0x00000200

#define GPIOEX_YARD	(GPIOEX_YARD_LEFT | GPIOEX_YARD_RIGHT | \
			 GPIOEX_YARD_FRONT | GPIOEX_YARD_BACK)

int gpioex_init(void);
int gpioex_set(uint32_t gpio, int value);
int gpioex_get_barrel_level(void);

#endif /*__GPIOEX_H__*/
