/*
 * dl_module.c
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

#include "dl_module.h"
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <mosquitto.h>
#include "logging.h"

int dlm_create(dlm_head_t *head, const char *filename)
{
	int ret = 0;
	const char *dl_err = NULL;
	struct garden_module* (*create_garden_module)(enum loglevel);
	dlm_t *dlm = calloc(1, sizeof(dlm_t));

	if (!dlm) {
		log_err("Allocate memory for dlm failed");
		ret = -ENOMEM;
		goto err;
	}

	if (!filename || !*filename) {
		log_err("Invalid module filename");
		ret = -EINVAL;
		goto err_free_dlm;
	}

	if (!head) {
		log_err("Invalid module list");
		ret = -EINVAL;
		goto err_free_dlm;
	}

	dlerror();
	dlm->dl_handler = dlopen(filename, RTLD_NOW);
	dl_err = dlerror();
	if (dl_err) {
		log_err("dlopen for %s failed %s", filename, dl_err);
		ret = -EINVAL;
		goto err_free_dlm;
	}

	create_garden_module = dlsym(dlm->dl_handler, "create_garden_module");
	dl_err = dlerror();
	if (dl_err) {
		log_dbg("%s ignored. No get_garden_module function found",
			filename);
		ret = 1;
		goto err_dlclose;
	}

	dlm->destroy_garden_module = dlsym(dlm->dl_handler,
					   "destroy_garden_module");
	dl_err = dlerror();
	if (dl_err) {
		log_dbg("%s ignored. No destroy_garden_module function found",
			filename);
		ret = 1;
		goto err_dlclose;
	}

	dlm->garden = create_garden_module(max_loglevel);
	if (!dlm->garden) {
		log_err("creation of garden module for %s failed", filename);
		ret = -ENOMEM;
		goto err_dlclose;
	}

	LIST_INSERT_HEAD(head, dlm, dl_modules);

	return 0;
err_dlclose:
	dlclose(dlm->dl_handler);
err_free_dlm:
	free(dlm);
err:
	return ret;
}

void dlm_destroy(dlm_head_t *head)
{
	while (head->lh_first != NULL) {
		dlm_t *dlm = head->lh_first;

		if (dlm->destroy_garden_module)
			dlm->destroy_garden_module(dlm->garden);

		if (dlm->dl_handler)
			dlclose(dlm->dl_handler);
		LIST_REMOVE(dlm, dl_modules);
		free(dlm);
	}
}

int dlm_mod_init(dlm_head_t *head, const char *conf_file,
		 struct mosquitto *mosq)
{
	int ret = 0;
	dlm_t *dlm = NULL;

	for (dlm = head->lh_first; dlm != NULL; dlm = dlm->dl_modules.le_next) {
		if (dlm->garden && dlm->garden->init) {
			ret = dlm->garden->init(dlm->garden, conf_file, mosq);
			if (ret < 0)
				break;
		}
	}

	return ret;
}

int dlm_mod_subscribe(dlm_head_t *head)
{
	int ret = 0;

	dlm_t *dlm = NULL;

	for (dlm = head->lh_first; dlm != NULL; dlm = dlm->dl_modules.le_next) {
		if (dlm->garden && dlm->garden->subscribe) {
			ret = dlm->garden->subscribe(dlm->garden);
			if (ret)
				break;
		}
	}

	return ret;
}

int dlm_mod_message(dlm_head_t *head, const struct mosquitto_message *message)
{
	int ret = 0;

	dlm_t *dlm = NULL;

	for (dlm = head->lh_first; dlm != NULL; dlm = dlm->dl_modules.le_next) {
		if (dlm->garden && dlm->garden->message) {
			ret = dlm->garden->message(dlm->garden, message);
			if (ret)
				break;
		}
	}

	return ret;
}
