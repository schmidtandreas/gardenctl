/*
 * mqtt.h
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
