#include "mqtt.h"
#include <string.h>
#include <errno.h>
#include <mosquitto.h>

#include "logging.h"

static void mqtt_on_log(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	log_dbg("MQTT: %s", str);
}

static void mqtt_on_connect(struct mosquitto *mosq, void *obj, int result)
{
	if (result)
	{
		log_err("MQTT connection failed (%d) %s", result, mosquitto_strerror(result));
		return;
	}

	log_dbg("MQTT client connected");

	mosquitto_subscribe(mosq, NULL, "/garden/test", 2);
}

static void mqtt_on_subscribe(struct mosquitto *mosq, void *obj, int mid,
			      int qos_count, const int *granted_qos)
{
	int i;

	log_dbg("MQTT subscribed (mid: %d): [0] %d", mid, granted_qos[0]);
		for(i = 1; i < qos_count; i++){
			log_dbg("MQTT subscribed (mid %d): [%d] %d", mid, i, granted_qos[i]);
		}
}

static void mqtt_on_message(struct mosquitto *mosq, void *obj, 
			    const struct mosquitto_message *message)
{
	log_dbg("MQTT [%s] message received:\n %s", message->topic, message->payload);
}

int mqtt_run(const char *username, const char *password)
{
	int ret = 0;
	struct mqtt mqtt;

	memset(&mqtt, 0, sizeof(struct mqtt));

	ret = mosquitto_lib_init();
	if (ret != MOSQ_ERR_SUCCESS)
		goto out;
	
	mqtt.mosq = mosquitto_new("gardenctl", true, &mqtt);
	if (mqtt.mosq == NULL) {
		ret = errno;
		log_err("create mosquitto object failed (%d) %s", ret, strerror(ret));
		goto out_cleanup;
	}

	log_dbg("MQTT client created");

	//TODO: set last will if neseccary

	ret = mosquitto_username_pw_set(mqtt.mosq, username, password);
	if (ret != MOSQ_ERR_SUCCESS) {
		log_err("set mosquitto username and passowrd failed (%d) %s", ret,
				strerror(ret));
		goto out_destroy;
	}

	log_dbg("MQTT set username: %s pass: %s", username, password);

	mosquitto_log_callback_set(mqtt.mosq, mqtt_on_log);
	mosquitto_connect_callback_set(mqtt.mosq, mqtt_on_connect);
	mosquitto_subscribe_callback_set(mqtt.mosq, mqtt_on_subscribe);
	mosquitto_message_callback_set(mqtt.mosq, mqtt_on_message);

	log_dbg("MQTT client iniated");

	ret = mosquitto_connect(mqtt.mosq, "homeserver", 1883, 30);
	if (ret != MOSQ_ERR_SUCCESS) {
		log_err("connect to mqtt broker failed (%d) %s", ret, strerror(ret));
		goto out_destroy;
	}

	mosquitto_loop_forever(mqtt.mosq, -1, 1);
	log_dbg("MQTT client disconnected");
out_destroy:
	mosquitto_destroy(mqtt.mosq);
	log_dbg("MQTT client destroied");
out_cleanup:
	mosquitto_lib_cleanup();
out:
	return ret;
}
