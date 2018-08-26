/* 
 * dl_module.h
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

#ifndef __DL_MODULE_H__
#define __DL_MODULE_H__

#include "garden_module.h"
#include <sys/queue.h>

typedef struct dl_module {
	void *dl_handler;
	struct garden_module *garden;
	void (*destroy_garden_module)(struct garden_module*);
	LIST_ENTRY(dl_module) dl_modules;
} dlm_t;

typedef LIST_HEAD(dl_module_s, dl_module) dlm_head_t;

int dlm_create(dlm_head_t *head, const char *filename);
void dlm_destroy(dlm_head_t *head);

int dlm_mod_init(dlm_head_t *head, struct mosquitto *mosq);
int dlm_mod_subscribe(dlm_head_t *head);
int dlm_mod_message(dlm_head_t *head, const struct mosquitto_message *message);

#endif /*__DL_MODULE_H__*/
