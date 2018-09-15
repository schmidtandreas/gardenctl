/*
 * mock_dl.h
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

#ifndef __MOCK_DL_H__
#define __MOCK_DL_H__

void mock_dlopen(const char *dl_filename, void *dl_handle,
		 const char *dl_error);
void mock_dlsym(void *dl_handle, const char *symname, void *ret,
		const char *dl_error);
void mock_dlclose(void *dl_handle, int ret);

#endif /*__MOCK_DL_H__*/
