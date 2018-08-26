#ifndef __ARGUMENTS_H__
#define __ARGUMENTS_H__

#include <stdbool.h>
#include <stdint.h>

struct arguments {
	const char *app_name;
	bool daemonize;
	const char *pidfile;
	const char *moddir;
	const char *conf_file;
	struct{
		const char *host;
		uint16_t port;
		const char *user;
		const char *pass;
		const char *passfile;
	}mqtt;
};

#endif /*__ARGUMENTS_H__*/
