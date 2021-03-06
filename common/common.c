/*
 * common.c
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

#include <garden_common.h>
#include <errno.h>
#include <string.h>
#include <regex.h>

int payload_on_off_to_int(const char *payload)
{
	size_t len;
	int ret = -EINVAL;

	if (!payload)
		return ret;

	len = strlen(payload);

	if (len >= 2 && len <= 3) {
		if (strcmp("on", payload) == 0)
			ret = 1;
		else if (strcmp("off", payload) == 0)
			ret = 0;
	}

	return ret;
}

int match_regex(const char *pattern, const char *string)
{
	int ret = 0;
	regex_t re;

	ret = regcomp(&re, pattern, REG_ICASE | REG_EXTENDED);
	if (ret < 0)
		return 0;

	ret = regexec(&re, string, (size_t)0, NULL, 0);
	regfree(&re);

	if (ret != 0)
		return 0;

	return 1;
}
