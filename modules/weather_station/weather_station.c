/* 
 * weather_station.c
 * This file is a part of gardenctl
 *
 * Copyright (C) 2018 Andreas Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <garden_module.h>
#include <garden_common.h>
#include <logging.h>
#include <gpioex.h>

#define INTERVAL_SEC 30
#define PUBLISH_INT 10

#define DHT_TEMP_FILENAME "/sys/bus/iio/devices/iio:device0/in_temp_input"
#define DHT_HUMREL_FILENAME "/sys/bus/iio/devices/iio:device0/in_humidityrelative_input"

enum thread_state {
	THREAD_STATE_STOPPED,
	THREAD_STATE_RUNNING,
};

struct weather_station {
	pthread_t thread;
	pthread_mutex_t lock;
	enum thread_state thread_state;
};

static int get_dht_temp(double *temp)
{
	int ret = 0;
	int fd;
	char buffer[20] = {0};
	*temp = 0.0;

	ret = open(DHT_TEMP_FILENAME, O_RDONLY);
	if (ret < 0)
		goto out;

	fd = ret;

	ret = read(fd, buffer, sizeof(buffer));
	if (ret < 0)
		goto out_close;

	*temp = atoi(buffer);
	*temp /= 1000;

	log_dbg("dht temp: %.1f", *temp);
out_close:
	close(fd);
out:
	return ret;
}

static int get_dht_hum(double *hum)
{
	int ret = 0;
	int fd;
	char buffer[20] = {0};
	*hum = 0.0;

	ret = open(DHT_HUMREL_FILENAME, O_RDONLY);
	if (ret < 0)
		goto out;

	fd = ret;

	ret = read(fd, buffer, sizeof(buffer));
	if (ret < 0)
		goto out_close;

	*hum = atoi(buffer);
	*hum /= 1000;

	log_dbg("dht hum: %.1f", *hum);
out_close:
	close(fd);
out:
	return ret;
}

static void* gm_thread_run(void *obj)
{
	struct garden_module *gm = (struct garden_module*) obj;
	struct weather_station *data = (struct weather_station*) gm->data;
	enum thread_state state;
	double dht_temp = 0;
	double dht_hum = 0;
	uint32_t period = 0;

	state = data->thread_state = THREAD_STATE_RUNNING;

	while (state == THREAD_STATE_RUNNING) {
		int ret = 0;
		double curr_dht_temp, curr_dht_hum;

		ret = get_dht_temp(&curr_dht_temp);
		if (ret >= 0 && (dht_temp != curr_dht_temp || !(period % PUBLISH_INT))) {
			char buf[10] = {0};
			dht_temp = curr_dht_temp;

			sprintf(buf, "%.1f", dht_temp);

			ret = mosquitto_publish(gm->mosq, NULL, "/garden/sensor/temperature",
						strlen(buf), buf, 2, false);
			if (ret < 0) {
				log_err("publish dht temperature failed (%d) %s", ret,
					mosquitto_strerror(ret));
			}
		}

		ret = get_dht_hum(&curr_dht_hum);
		if (ret >= 0 && (dht_hum != curr_dht_hum || !(period % PUBLISH_INT))) {
			char buf[10] = {0};
			dht_hum = curr_dht_hum;

			sprintf(buf, "%.1f", dht_hum);

			ret = mosquitto_publish(gm->mosq, NULL, "/garden/sensor/humidity",
						strlen(buf), buf, 2, false);
			if (ret < 0) {
				log_err("publish dht humidity failed (%d) %s", ret,
					mosquitto_strerror(ret));
			}
		}	

		sleep(INTERVAL_SEC);
		pthread_mutex_lock(&data->lock);
		state = data->thread_state;
		pthread_mutex_unlock(&data->lock);

		++period;
	}

	log_dbg("exit weather station thread");

	return NULL;
}

static void gm_thread_stop(struct weather_station* data)
{
	int ret = 0;

	if (data) {
		log_dbg("stopping weather station thread");
		pthread_mutex_lock(&data->lock);
		data->thread_state = THREAD_STATE_STOPPED;
		pthread_mutex_unlock(&data->lock);

		pthread_join(data->thread, NULL);
		log_dbg("stopped weather station thread");
	}
}

static int gm_init(struct garden_module *gm, struct mosquitto* mosq)
{
	int ret = 0;
	struct weather_station *data = NULL;

	gm->mosq = mosq;
	gm->data = malloc(sizeof(struct weather_station));

	if (!gm->data) {
		log_err("allocation for weather station data failed");
		ret = -ENOMEM;
		goto out;
	}

	data = (struct weather_station*)gm->data;

	ret = pthread_mutex_init(&data->lock, NULL);
	if (ret) {
		log_err("initiate weather station lock failed (%d) %s", ret, strerror(ret));
		goto out;
	}

	ret = pthread_create(&data->thread, NULL, &gm_thread_run, gm);
	if (ret) {
		log_err("create weather station thread failed (%d) %s", ret, strerror(ret));
		goto out;
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
