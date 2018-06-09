#include <stdlib.h>
#include <config.h>
#include <string.h>
#include <errno.h>
#include <garden_module.h>
#include <garden_common.h>
#include <logging.h>
#include <gpioex.h>

enum topics {
	TOPIC_LIGHT_TREE,
	TOPIC_LIGHT_HOUSE,
	MAX_TOPICS,
};

struct light {
	const char *topics[MAX_TOPICS];
};

static int gm_init(struct garden_module *gm, struct mosquitto* mosq)
{
	int ret = 0;
	struct light *data = NULL;

	gm->mosq = mosq;
	gm->data = malloc(sizeof(struct light));

	if (!gm->data) {
		log_err("allocation for light data failed");
		ret = -ENOMEM;
		goto out;
	}

	data = (struct light*)gm->data;

	data->topics[TOPIC_LIGHT_TREE] = "/garden/light/tree";
	data->topics[TOPIC_LIGHT_HOUSE] = "/garden/light/house";
out:
	return ret;
}

static int gm_subscribe(struct garden_module *gm)
{
	int ret = 0;
	struct light *data = (struct light*)gm->data;

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

static int gm_message(struct garden_module *gm, const struct mosquitto_message *message)
{
	int ret = 0;
	struct light *data;

	log_dbg("light: handle message - topic: %s", message->topic);
	log_dbg("light: message content: %s", message->payload);

	data = gm->data;

	if (!data) {
		ret = -EINVAL;
		goto out;
	}

	if (strcmp(data->topics[TOPIC_LIGHT_TREE], message->topic) == 0) {
		ret = payload_on_off_to_int(message->payload, message->payloadlen);
		if (ret < 0)
			goto out;
		ret = gpioex_set(GPIOEX_LIGHT_TREE, ret);
	} else if (strcmp(data->topics[TOPIC_LIGHT_HOUSE], message->topic) == 0) {
		ret = payload_on_off_to_int(message->payload, message->payloadlen);
		if (ret < 0)
			goto out;
		ret = gpioex_set(GPIOEX_LIGHT_HOUSE, ret);
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
		if (module->data)
			free(module->data);
		free(module);
	}
}
