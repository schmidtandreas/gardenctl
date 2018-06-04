#ifndef __MQTT_H__
#define __MQTT_H__

#include <mosquitto.h>

struct mqtt {
	struct mosquitto *mosq;
};

int mqtt_run(const char *username, const char *password);
#endif /*__MQTT_H__*/
