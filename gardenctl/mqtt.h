#ifndef __MQTT_H__
#define __MQTT_H__

#include <mosquitto.h>
#include "dl_module.h"
#include "arguments.h"

enum mqtt_state {
	MQTT_STATE_DISCONNECTED = 0,
	MQTT_STATE_CONNECTED,
};

struct mqtt {
	struct mosquitto *mosq;
	dlm_head_t *dlm_head;
	enum mqtt_state state;
};

int mqtt_run(dlm_head_t* dlm_head, struct arguments *args);
void mqtt_quit(void);

#endif /*__MQTT_H__*/
