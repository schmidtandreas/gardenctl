#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <garden_module.h>
#include <garden_common.h>
#include <logging.h>
#include <gpioex.h>

#define GET_BARREL_LVL_INTERVAL_SEC 5
#define PUBLISH_INT 10
#define POLL_TAP_BTN_TIMEOUT 30000

#define GPIO_TAP_BTN 26

enum topics {
	TOPIC_TAP,
	TOPIC_WATER,
	TOPIC_DROPPIPE_PUMP,
	TOPIC_BARREL,
	MAX_TOPICS,
};

enum thread_state {
	THREAD_STATE_STOPPED,
	THREAD_STATE_STARTING,
	THREAD_STATE_RUNNING,
};

struct watering {
	const char *topics[MAX_TOPICS];
	pthread_t thread_barrel;
	pthread_t thread_tap_btn;
	pthread_mutex_t lock;
	enum thread_state thread_barrel_state;
	enum thread_state thread_tap_btn_state;
	int tap_state;
	int tap_btn_released;
};

static inline void gm_set_thread_state(struct watering *data, enum thread_state *state, enum thread_state value) {
	pthread_mutex_lock(&data->lock);
	*state = value;
	pthread_mutex_unlock(&data->lock);
}

static inline enum thread_state gm_get_thread_state(struct watering *data, enum thread_state *state) {
	enum thread_state ret;
	pthread_mutex_lock(&data->lock);
	ret = *state;
	pthread_mutex_unlock(&data->lock);

	return ret;
}

static void* gm_thread_barrel_run(void *obj)
{
	struct garden_module *gm = (struct garden_module*) obj;
	struct watering *data = (struct watering*) gm->data;
	enum thread_state state;
	int barrel_level = -1;
	uint32_t period = 0;

	pthread_mutex_lock(&data->lock);
	if (data->thread_barrel_state == THREAD_STATE_STARTING)
		data->thread_barrel_state = THREAD_STATE_RUNNING;
	pthread_mutex_unlock(&data->lock);

	state = gm_get_thread_state(data, &data->thread_barrel_state);
	while (state == THREAD_STATE_RUNNING) {
		int curr_barrel_level = gpioex_get_barrel_level();

		log_dbg("read barrel level: %d", curr_barrel_level);

		if (curr_barrel_level >= 0 && 
		    (curr_barrel_level != barrel_level || !(period % PUBLISH_INT))) {
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

			snprintf(buf, sizeof(buf), "%.1f%", barrel_level_percent);

			ret = mosquitto_publish(gm->mosq, NULL, "/garden/sensor/barrel",
						strlen(buf), buf, 2, false);
			if (ret < 0) {
				log_err("publish barrel level failed (%d) %s", ret,
					mosquitto_strerror(ret));
			}
		}

		sleep(GET_BARREL_LVL_INTERVAL_SEC);
		state = gm_get_thread_state(data, &data->thread_barrel_state);

		++period;
	}

	log_dbg("exit watering thread for barrel");
	return NULL;
}

static int gm_debounce_gpio(int fd, char *value)
{
	int ret = 0;
	char tmp_val;
	uint8_t counter = 18;

	lseek(fd, 0, SEEK_SET);
	ret = read(fd, value, 1);

	while(counter--) {
		lseek(fd, 0, SEEK_SET);
		ret = read(fd, &tmp_val, 1);
		if (ret > 0) {
			if (*value != tmp_val) {
				counter = 0;
				*value = tmp_val;
			}
		} else {
			log_err("read gpio value failed (%d) %s", ret, strerror(ret));
			ret = (ret) ? :-ENOENT;
		}

		usleep(10000);
	}

	return ret;
}

static void gm_get_tap_value(struct watering *data, char gpio_val, char *tap_val, size_t size)
{
	if (gpio_val == '0') {
		if (data->tap_btn_released)
			data->tap_state = !data->tap_state;
		data->tap_btn_released = 0;
	} else if (gpio_val == '1')
		data->tap_btn_released = 1;

	if (data->tap_state)
		snprintf(tap_val, size, "on");
	else
		snprintf(tap_val, size, "off");

	log_dbg("tap state: %s (gpio: %c)", tap_val, gpio_val);
}

static void* gm_thread_tap_btn_run(void *obj)
{
	struct garden_module *gm = (struct garden_module*) obj;
	struct watering *data = (struct watering*) gm->data;
	enum thread_state state;
	char gpio_val_path[100] = {0};
	int gpio = GPIO_TAP_BTN;
	int fd, ret;
	struct pollfd pfd;

	pthread_mutex_lock(&data->lock);
	if (data->thread_tap_btn_state == THREAD_STATE_STARTING)
		data->thread_tap_btn_state = THREAD_STATE_RUNNING;
	pthread_mutex_unlock(&data->lock);

	snprintf(gpio_val_path, sizeof(gpio_val_path), "/sys/class/gpio/gpio%d/value", gpio);

	ret = open(gpio_val_path, O_RDONLY);
	if (ret < 0) {
		log_err("open gpio value failed (%d) %s", ret, strerror(ret));
		gm_set_thread_state(data, &data->thread_tap_btn_state, THREAD_STATE_STOPPED);
		return NULL;
	}

	pfd.fd = fd = ret;
	pfd.events = POLLPRI;

	state = gm_get_thread_state(data, &data->thread_tap_btn_state);
	while (state == THREAD_STATE_RUNNING) {
		lseek(fd, 0, SEEK_SET);
		ret = poll(&pfd, 1, POLL_TAP_BTN_TIMEOUT);
		if (ret >= 0) {
			char gpio_val;
			if (gm_debounce_gpio(fd, &gpio_val) >= 0) {
				char tap_val[4] = {0};
				gm_get_tap_value(data, gpio_val, tap_val, sizeof(tap_val));

				if (*tap_val)
					ret = mosquitto_publish(gm->mosq, NULL, "/garden/sensor/tap",
								strlen(tap_val), tap_val, 2, false);
				if (ret < 0) {
					log_err("publish tap btn value failed (%d) %s", ret,
						mosquitto_strerror(ret));
				}
			}
		} else
			log_err("poll for gpio value failed (%d) %s", ret, strerror(ret));

		state = gm_get_thread_state(data, &data->thread_tap_btn_state);
	}

	close(fd);
	log_dbg("exit watering thread for tap button");

	return NULL;
}

static void gm_thread_stop(struct watering* data)
{
	int ret = 0;

	if (data) {
		log_dbg("stopping watering threads");
		gm_set_thread_state(data, &data->thread_barrel_state, THREAD_STATE_STOPPED);
		gm_set_thread_state(data, &data->thread_tap_btn_state, THREAD_STATE_STOPPED);

		if (data->thread_barrel)
			pthread_join(data->thread_barrel, NULL);
		if (data->thread_tap_btn)
			pthread_join(data->thread_tap_btn, NULL);
		data->thread_barrel = 0;
		data->thread_tap_btn = 0;
		log_dbg("watering threads stopped");
	}
}

static int gm_export_gpio(int gpio)
{
	int ret = 0;
	int fd = 0;
	int len = 0;
	char buf[20] = {0};
	char path[100] = {0};
	struct stat st;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", gpio);
	ret = lstat(path, &st);

	if (ret && errno == ENOENT) {
		ret = open("/sys/class/gpio/export", O_WRONLY);
		if (ret < 0) {
			log_err("open gpio export failed (%d) %s", ret, strerror(ret));
			goto out;
		}

		fd = ret;

		len = snprintf(buf, sizeof(buf), "%d", gpio);
		if (len <= 0) {
			log_err("create gpio number string failed (%d) %s", len, strerror(len));
			close(fd);
			goto out;
		}

		ret = write(fd, buf, len);
		if (ret < 0)
			log_err("write to gpio export failed (%d) %s", ret, strerror(ret));

		close(fd);
	} else if (ret)
		log_err("check link %s failed (%d) %s", path, errno, strerror(errno));
	else
		log_dbg("gpio %s aleady exported", path);
out:
	return (ret < 0) ? : 0;

}

static int gm_set_gpio_direction(int gpio, const char *direction)
{
	int ret = 0;
	int fd = 0;
	char path[100] = {0};

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);

	ret = open(path, O_WRONLY);
	if (ret < 0) {
		log_err("open gpio direction failed (%d) %s", ret, strerror(ret));
		goto out;
	}

	fd = ret;

	ret = write(fd, direction, strlen(direction));
	if (ret < 0)
		log_err("write to gpio direction failed (%d) %s", ret, strerror(ret));
	
	close(fd);

	log_dbg("set gpio direction \"%s\" to %s", direction, path);
out:
	return (ret < 0) ? : 0;
}

static int gm_set_gpio_edge(int gpio, const char *edge)
{
	int ret = 0;
	int fd = 0;
	char path[100] = {0};

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", gpio);

	ret = open(path, O_WRONLY);
	if (ret < 0) {
		log_err("open gpio edge failed (%d) %s", ret, strerror(ret));
		goto out;
	}

	fd = ret;

	ret = write(fd, edge, strlen(edge));
	if (ret < 0)
		log_err("write to gpio edge failed (%d) %s", ret, strerror(ret));
	
	close(fd);

	log_dbg("set gpio edge \"%s\" to %s", edge, path);
out:
	return (ret < 0) ? : 0;
}

static int gm_init_tap_btn(struct watering *data)
{
	int ret = 0;
	ret = gm_export_gpio(GPIO_TAP_BTN);
	if (ret)
		goto out;

	ret = gm_set_gpio_direction(GPIO_TAP_BTN, "in");
	if (ret)
		goto out;

	ret = gm_set_gpio_edge(GPIO_TAP_BTN, "both");
	if (ret)
		goto out;

out:
	return ret;
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

	gm_set_thread_state(data, &data->thread_barrel_state, THREAD_STATE_STARTING);
	ret = pthread_create(&data->thread_barrel, NULL, &gm_thread_barrel_run, gm);
	if (ret) {
		log_err("create watering thread for barrel failed (%d) %s", ret, strerror(ret));
		goto out;
	}

	ret = gm_init_tap_btn(data);
	if (ret) {
		gm_thread_stop(data);
		goto out;
	}

	gm_set_thread_state(data, &data->thread_tap_btn_state, THREAD_STATE_STARTING);
	ret = pthread_create(&data->thread_tap_btn, NULL, &gm_thread_tap_btn_run, gm);
	if (ret) {
		log_err("create watering thread for tap button failed (%d) %s", ret, strerror(ret));
		gm_thread_stop(data);
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
