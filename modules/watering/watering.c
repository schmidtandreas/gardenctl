#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <garden_module.h>
#include <garden_common.h>
#include <logging.h>
#include <gpioex.h>

#define GET_BARREL_LVL_INTERVAL_SEC 5

enum topics {
	TOPIC_TAP,
	TOPIC_WATER,
	TOPIC_DROPPIPE_PUMP,
	TOPIC_BARREL,
	MAX_TOPICS,
};

enum thread_state {
	THREAD_STATE_STOPPED,
	THREAD_STATE_RUNNING,
};

struct watering {
	const char *topics[MAX_TOPICS];
	pthread_t thread;
	pthread_mutex_t lock;
	enum thread_state thread_state;
};

static void* gm_thread_run(void *obj)
{
	struct garden_module *gm = (struct garden_module*) obj;
	struct watering *data = (struct watering*) gm->data;
	enum thread_state state;
	int barrel_level = -1;

	state = data->thread_state = THREAD_STATE_RUNNING;

	while (state == THREAD_STATE_RUNNING) {
		int curr_barrel_level = gpioex_get_barrel_level();

		log_dbg("read barrel level: %d", curr_barrel_level);

		if (curr_barrel_level >= 0 && curr_barrel_level != barrel_level) {
			int ret = 0;
			int i = 0;
			char buf[20] = {0};
			double barrel_level_percent = 0;

			barrel_level = curr_barrel_level;

			for (i = 0; i < 8; ++i) {
				if (!(curr_barrel_level & 0x1))
					barrel_level_percent += 12.5;
				curr_barrel_level >>= 1;
			}

			sprintf(buf, "%.1f%", barrel_level_percent);

			ret = mosquitto_publish(gm->mosq, NULL, "/garden/sensor/barrel",
						strlen(buf), buf, 2, false);
			if (ret < 0) {
				log_err("publish barrel level failed (%d) %s", ret,
					mosquitto_strerror(ret));
			}
		}

		sleep(GET_BARREL_LVL_INTERVAL_SEC);
		pthread_mutex_lock(&data->lock);
		state = data->thread_state;
		pthread_mutex_unlock(&data->lock);
	}

	return NULL;
}

static void gm_thread_stop(struct watering* data)
{
	int ret = 0;

	if (data) {
		pthread_mutex_lock(&data->lock);
		data->thread_state = THREAD_STATE_STOPPED;
		pthread_mutex_unlock(&data->lock);

		pthread_join(data->thread, NULL);
	}
}

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

	ret = pthread_mutex_init(&data->lock, NULL);
	if (ret) {
		log_err("initiate watering lock failed (%d) %s", ret, strerror(ret));
		goto out;
	}

	ret = pthread_create(&data->thread, NULL, &gm_thread_run, gm);
	if (ret) {
		log_err("create watering thread failed (%d) %s", ret, strerror(ret));
		goto out;
	}
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
		goto out;
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

struct garden_module *create_garden_module(enum loglevel loglevel)
{
	struct garden_module *module = malloc(sizeof(*module));

	if (module) {
		memset(module, 0, sizeof(*module));
		module->init = gm_init;
		module->subscribe = gm_subscribe;
		module->message = gm_message;

		max_loglevel = loglevel;
	}

	return module;
}

void destroy_garden_module(struct garden_module *module)
{
	if (module) {
		gm_thread_stop(module->data);
		if (module->data)
			free(module->data);
		free(module);
	}
}
