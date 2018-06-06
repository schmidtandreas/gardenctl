#include <stdlib.h>
#include <config.h>
#include <string.h>
#include <errno.h>
#include <garden_module.h>
#include <garden_common.h>
#include <logging.h>
#include <gpioex.h>

enum topics {
	TOPIC_TAP,
	TOPIC_WATER,
	TOPIC_DROPPIPE_PUMP,
	TOPIC_BARREL,
	MAX_TOPICS,
};

struct watering {
	const char *topics[MAX_TOPICS];
};

static int gm_init(struct garden_module *gm, struct mosquitto* mosq)
{
	int ret = 0;
	struct watering *data = NULL;

	gm->mosq = mosq;
	gm->data = malloc(sizeof(struct watering));

	if (!gm->data) {
		log_err("allocation for watering data failed");
		ret = -ENOMEM;
		goto out;
	}

	data = (struct watering*)gm->data;

	data->topics[TOPIC_TAP] = "/garden/tap";
	data->topics[TOPIC_WATER] = "/garden/water";
	data->topics[TOPIC_DROPPIPE_PUMP] = "/garden/droppipepump";
	data->topics[TOPIC_BARREL] = "/garden/barrel";
out:
	return ret;
}

static int gm_subscribe(struct garden_module *gm)
{
	int ret = 0;
	struct watering *data = (struct watering*)gm->data;

	if (data) {
		int i = 0;
		for (i = 0; i < MAX_TOPICS; ++i) {
			ret = mosquitto_subscribe(gm->mosq, NULL, data->topics[i], 2);
			if (ret) {
				log_err("subscribe %s failed (%d) %s", data->topics[i],
					ret, mosquitto_strerror(ret));
				break;
			}
		}
	}

	return ret;
}

static int water_payload_to_gpioex(const char *payload, size_t len, uint32_t *gpio, int *val)
{
	int ret = 0;

	if (!len) {
		ret = -EINVAL;
		goto out;
	}

	if (strcmp("left", payload) == 0) {
		*gpio = GPIOEX_YARD_LEFT;
		*val = 1;
	} else if (strcmp("right", payload) == 0) {
		*gpio = GPIOEX_YARD_RIGHT;
		*val = 1;
	} else if (strcmp("middle", payload) == 0) {
		*gpio = GPIOEX_YARD_LEFT | GPIOEX_YARD_RIGHT;
		*val = 1;
	} else if (strcmp("front", payload) == 0) {
		*gpio = GPIOEX_YARD_FRONT;
		*val = 1;
	} else if (strcmp("back", payload) == 0) {
		*gpio = GPIOEX_YARD_BACK;
		*val = 1;
	} else if (strcmp("front_back", payload) == 0) {
		*gpio = GPIOEX_YARD_FRONT | GPIOEX_YARD_BACK;
		*val = 1;
	} else if (strcmp("off", payload) == 0) {
		*gpio = GPIOEX_YARD_LEFT | GPIOEX_YARD_RIGHT;
		*gpio |= GPIOEX_YARD_FRONT | GPIOEX_YARD_BACK;
		*val = 0;
	}
out:
	return ret;
}

static int gm_message(struct garden_module *gm, const struct mosquitto_message *message)
{
	int ret = 0;
	struct watering *data;

	log_dbg("watering: handle message - topic: %s", message->topic);
	log_dbg("watering: message content: %s", message->payload);

	data = gm->data;

	if (!data) {
		ret = -EINVAL;
	}

	if (strcmp(data->topics[TOPIC_TAP], message->topic) == 0) {
		ret = payload_on_off_to_int(message->payload, message->payloadlen);
		if (ret < 0)
			goto out;
		ret = gpioex_set(GPIOEX_TAP, ret);
	} else if (strcmp(data->topics[TOPIC_WATER], message->topic) == 0) {
		int gpio, val;
		ret = water_payload_to_gpioex(message->payload, message->payloadlen, &gpio, &val);
		if (ret < 0)
			goto out;
		ret = gpioex_set(gpio, val);
	} else if (strcmp(data->topics[TOPIC_DROPPIPE_PUMP], message->topic) == 0) {
		ret = payload_on_off_to_int(message->payload, message->payloadlen);
		if (ret < 0)
			goto out;
		ret = gpioex_set(GPIOEX_DROPPIPE_PUMP, ret);
	} else if (strcmp(data->topics[TOPIC_BARREL], message->topic) == 0) {
		ret = payload_on_off_to_int(message->payload, message->payloadlen);
		if (ret < 0)
			goto out;
		ret = gpioex_set(GPIOEX_BARREL, ret);
	}
out:
	return ret;
}

struct garden_module *create_garden_module(void)
{
	struct garden_module *module = malloc(sizeof(*module));

	if (module) {
		memset(module, 0, sizeof(*module));
		module->init = gm_init;
		module->subscribe = gm_subscribe;
		module->message = gm_message;
	}

	return module;
}

void destroy_garden_module(struct garden_module *module)
{
	if (module) {
		if (module->data)
			free(module->data);
		free(module);
	}
}
