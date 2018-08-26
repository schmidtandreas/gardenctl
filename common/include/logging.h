/* 
 * logging.h
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

#ifndef __LOGGING_H__
#define __LOGGING_H__

enum loglevel {
	LOGLEVEL_INFO,
	LOGLEVEL_ERROR,
	LOGLEVEL_WARNING,
	LOGLEVEL_DEBUG,
};

extern enum loglevel max_loglevel;

extern int logging(enum loglevel level, const char *fmt, ...);

#define log_info(__fmt, ...) logging(LOGLEVEL_INFO, (__fmt), ##__VA_ARGS__)
#define log_err(__fmt, ...) logging(LOGLEVEL_ERROR, (__fmt), ##__VA_ARGS__)
#define log_warn(__fmt, ...) logging(LOGLEVEL_WARNING, (__fmt), ##__VA_ARGS__)
#define log_dbg(__fmt, ...) logging(LOGLEVEL_DEBUG, (__fmt), ##__VA_ARGS__)

#endif /*__LOGGING_H__*/
