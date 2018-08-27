/* 
 * garden_module.h
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

#ifndef __GARDEN_MODULE_H__
#define __GARDEN_MODULE_H__

#include <mosquitto.h>

struct garden_module {
	int (*init)(struct garden_module*, const char *conf_file, struct mosquitto*);
	int (*subscribe)(struct garden_module*);
	int (*message)(struct garden_module*, const struct mosquitto_message *);

	struct mosquitto *mosq;
	void *data;
};

#endif /*__GARDEN_MODULE_H__*/
