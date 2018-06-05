#ifndef __GARDEN_MODULE_H__
#define __GARDEN_MODULE_H__

#include <mosquitto.h>

struct garden_module {
	int (*init)(struct garden_module*, struct mosquitto*);
	int (*subscribe)(struct garden_module*);
	int (*message)(struct garden_module*, const struct mosquitto_message *);

	struct mosquitto *mosq;
	void *data;
};

#endif /*__GARDEN_MODULE_H__*/
