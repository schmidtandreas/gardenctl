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
