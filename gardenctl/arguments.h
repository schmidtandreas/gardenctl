#ifndef __ARGUMENTS_H__
#define __ARGUMENTS_H__

#include <stdbool.h>

struct arguments {
	const char *app_name;
	bool daemonize;
	const char *pidfile;
	const char *moddir;
	const char *mqtt_user;
	const char *mqtt_pass;
	const char *mqtt_passfile;
};

#endif /*__ARGUMENTS_H__*/
