#ifndef __MQTT_H__
#define __MQTT_H__

#include <mosquitto.h>
#include "dl_module.h"

struct mqtt {
	struct mosquitto *mosq;
	dlm_head_t *dlm_head;
};

int mqtt_run(dlm_head_t* dlm_head, const char *username, const char *password);
#endif /*__MQTT_H__*/
