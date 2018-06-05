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
	dlm_t *dlm = malloc(sizeof(dlm_t));

	if (dlm) {
		const char *dl_err = NULL;

		memset(dlm, 0, sizeof(dlm_t));
		dlerror();
		dlm->dl_handler = dlopen(filename, RTLD_NOW);

		if ((dl_err = dlerror()) == NULL) {
			struct garden_module* (*create_garden_module)(void);

			create_garden_module = dlsym(dlm->dl_handler, "create_garden_module");
			if ((dl_err = dlerror()) != NULL) {
				log_dbg("%s ingnored no get_garden_module function found",
						filename);
				ret = 1;
				goto err_dlclose;
			}
			
			dlm->destroy_garden_module = dlsym(dlm->dl_handler, "destroy_garden_module");
			if ((dl_err = dlerror()) != NULL) {
				log_dbg("%s was ignored. No destroy_garden_module function found",
					filename);
				ret = 1;
				goto err_dlclose;
			}
			
			dlm->garden = create_garden_module();
			if (!dlm->garden) {
				log_err("creation of garden module for %s failed", filename);
				ret = -ENOMEM;
			}

			LIST_INSERT_HEAD(head, dlm, dl_modules);

		} else {
			log_err("dlopen for %s failed %s", filename, dl_err);
			ret = -EINVAL;
			goto err_free_dlm;
		}
	} else {
		log_err("Allocate memory for dlm failed");
		ret = -ENOMEM;
		goto err;
	}

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
	while(head->lh_first != NULL) {
		dlm_t *dlm = head->lh_first;
		
		if (dlm->destroy_garden_module)
			dlm->destroy_garden_module(dlm->garden);

		if (dlm->dl_handler)
			dlclose(dlm->dl_handler);
		LIST_REMOVE(dlm, dl_modules);
		free(dlm);
	}
}

int dlm_mod_init(dlm_head_t *head, struct mosquitto *mosq)
{
	int ret = 0;
	dlm_t *dlm = NULL;

	for (dlm = head->lh_first; dlm != NULL; dlm = dlm->dl_modules.le_next) {
		if (dlm->garden && dlm->garden->init) {
			ret = dlm->garden->init(dlm->garden, mosq);
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
		if (dlm->garden && dlm->garden->subscribe){
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
		if (dlm->garden && dlm->garden->message){
			ret = dlm->garden->message(dlm->garden, message);
			if (ret)
				break;
		}
	}

	return ret;
}
