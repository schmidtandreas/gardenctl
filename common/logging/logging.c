/*
 * logging.c
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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "logging.h"

#define MAX_FMT_SIZE 1000

__attribute__((visibility("hidden"))) enum loglevel max_loglevel = LOGLEVEL_DEBUG;

static int log_prefix(FILE *logfile, enum loglevel level)
{
	int ret = 0;
	const char *prefix = NULL;

	switch (level) {
	case LOGLEVEL_INFO:
		prefix = "-I- ";
		break;
	case LOGLEVEL_ERROR:
		prefix = "-E- ";
		break;
	case LOGLEVEL_WARNING:
		prefix = "-W- ";
		break;
	case LOGLEVEL_DEBUG:
	default:
		prefix = "-D- ";
		break;
	}

	if (prefix)
		ret = fprintf(logfile, "%s", prefix);

	return ret;
}

__attribute__((visibility("hidden"))) int logging(enum loglevel level, const char *fmt, ...)
{
	int ret = 0;
	va_list ap;
	FILE *logfile = stderr;
	size_t fmt_size = strnlen(fmt, MAX_FMT_SIZE);

	if (level > max_loglevel)
		goto out;

	if (fmt_size > 0 && fmt[fmt_size] != '\0') {
		fprintf(stderr, "-E- logging message to long\n");
		ret = -EINVAL;
		goto out;
	}

	ret = log_prefix(logfile, level);
	if (ret < 0)
		goto out;

	va_start(ap, fmt);
	ret = vfprintf(logfile, fmt, ap);
	va_end(ap);

	if (ret < 0)
		goto out;

	if (fmt_size > 0 && fmt[fmt_size - 1] != '\n')
		ret = fprintf(logfile, "\n");

out:
	return ret;
}
