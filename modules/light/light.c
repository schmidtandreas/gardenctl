#include <stdlib.h>
#include <config.h>
#include <string.h>
#include <errno.h>
#include <garden_module.h>
#include <logging.h>

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

	log_dbg("handle message - topic: %s", message->topic);
	log_dbg("message content: %s", message->payload);

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
