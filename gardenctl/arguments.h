/*
 * argumets.h
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

#ifndef __ARGUMENTS_H__
#define __ARGUMENTS_H__

#include <stdbool.h>
#include <stdint.h>

struct arguments {
	const char *app_name;
	bool daemonize;
	const char *pidfile;
	const char *moddir;
	const char *conf_file;
	struct {
		const char *host;
		uint16_t port;
		const char *user;
		const char *pass;
		const char *passfile;
	} mqtt;
};

#endif /*__ARGUMENTS_H__*/
