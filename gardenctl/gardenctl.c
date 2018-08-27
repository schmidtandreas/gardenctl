/*
 * gardenctl.c
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
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <logging.h>
#include <config.h>
#include <dirent.h>
#include <regex.h>
#include <linux/limits.h>
#include <confuse.h>
#include <gpioex.h>

#include "arguments.h"
#include "dl_module.h"
#include "mqtt.h"

static int match_regex(const char *pattern, const char *string)
{
	int ret = 0;
	regex_t re;

	ret = regcomp(&re, pattern, REG_ICASE | REG_EXTENDED);
	if (ret < 0)
		return 0;

	ret = regexec(&re, string, (size_t)0, NULL, 0);
	regfree(&re);

	if (ret != 0)
		return 0;

	return 1;
}

static int run(struct arguments *args)
{
	int ret = EXIT_SUCCESS;
	DIR *d = opendir(args->moddir);
	struct dirent *dir = NULL;
	dlm_head_t dlm_head;

	log_dbg("modules directory: %s", args->moddir);

	if (d == NULL) {
		log_err("Couldn't open %s directory.", args->moddir);
		ret = EXIT_FAILURE;
		goto out;
	}

	LIST_INIT(&dlm_head);

	while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_REG) {
			log_dbg("directory content: %s", dir->d_name);

			if (match_regex("libgarden_.*\\.so$", dir->d_name)) {
				char path[PATH_MAX];
				sprintf(path, "%s%s", args->moddir, dir->d_name);
				ret = dlm_create(&dlm_head, path);
				if (ret < 0)
					goto out_remove_dl_modules;
			}
		}
	}

	ret = mqtt_run(&dlm_head, args);

out_remove_dl_modules:
	dlm_destroy(&dlm_head);
	closedir(d);
out:
	return ret;
}

static int create_pidfile(const char *pidfilename)
{
	int ret = 0;
	FILE *pidfile = NULL;

	if (!pidfilename || !*pidfilename) {
		log_err("invalid pidfile");
		ret = -EINVAL;
		goto out;
	}

	pidfile = fopen(pidfilename, "w+");
	if (pidfile) {
		ret = fprintf(pidfile, "%d\n", (int)getpid());
		if (ret < 0)
			log_err("write pid to %s failed (%d) %s", pidfilename, errno,
				strerror(errno));

		if (fclose(pidfile)) {
			log_err("close pidfile %s failed (%d) %s",
				pidfilename, errno, strerror(errno));
			ret = -errno;
			goto out;
		}
	} else {
		log_err("open pidfile %s failed (%d) %s", pidfilename, errno, strerror(errno));
		ret = -errno;
		goto out;
	}

out:
	return ret;
}

static void args_set_default(int argc, char *argv[], struct arguments *args)
{
	memset(args, 0, sizeof(struct arguments));
	args->app_name = argv[0];
	args->daemonize = true;
}

static void args_free(struct arguments *args)
{
	free((void*)args->mqtt.host);
	free((void*)args->mqtt.user);
	free((void*)args->mqtt.pass);
	free((void*)args->mqtt.passfile);
}

static void usage(const char *app_name)
{
	fprintf(stdout,
		"Usage: %s [OPTIONS]\n"
		"Control garden features\n"
		"\n"
		"Options:\n"
		"  -h, --help                display this help and exit\n"
		"  -v, --version             display version and exit\n"
		"      --loglevel LEVEL      max. logging level\n"
		"                            (default is (%d)\n"
		"  -d, --no-daemonize        do not daemonize\n"
		"  -p, --pidfile FILE        specify path for pid file\n"
		"  -m, --mod-path PATH       specify path for modules\n"
		"  -c, --conffile FILE       specify path for configuration file\n"
		, app_name, max_loglevel);
}

static void pr_version(void)
{
	fprintf(stdout, "gardenctl (%s) %s\n", PACKAGE, VERSION);
}

static int conf_valid_loglevel(cfg_t *cfg, cfg_opt_t *opt)
{
	int ret = 0;

	int loglevel = cfg_opt_getnint(opt, 0);

	if (loglevel < LOGLEVEL_INFO || loglevel > LOGLEVEL_DEBUG) {
		cfg_error(cfg, "invalid loglevel %d it has to be between %d and %d\n",
			  loglevel, LOGLEVEL_INFO, LOGLEVEL_DEBUG);
		ret = -1;
	}

	return ret;
}

static int conf_valid_mqtt_passfile(cfg_t *cfg, cfg_opt_t *opt)
{
	int ret = 0;
	const char *filename = cfg_opt_getnstr(opt, 0);

	if (!filename || access(filename, R_OK) < 0) {
		cfg_error(cfg, "mqtt password file %s not exists or has no read access\n",
			  filename);
		ret = -1;
	}

	return ret;
}

static int conf_set_args_from_conf_file(struct arguments *args)
{
	int ret = 0;
	cfg_t *cfg;
	cfg_t *cfg_mqtt = NULL;

	cfg_opt_t mqtt_opts[] = {
		CFG_STR("host", "localhost", CFGF_NONE),
		CFG_INT("port", 1883, CFGF_NONE),
		CFG_STR("user", "", CFGF_NONE),
		CFG_STR("password", "", CFGF_NONE),
		CFG_STR("passfile", "", CFGF_NONE),
		CFG_END()
	};

	cfg_opt_t opts[] = {
		CFG_INT("loglevel", -1, CFGF_NONE),
		CFG_SEC("mqtt", mqtt_opts, CFGF_NONE),
		CFG_END()
	};

	cfg = cfg_init(opts, CFGF_NONE);

	cfg_set_validate_func(cfg, "loglevel", conf_valid_loglevel);
	cfg_set_validate_func(cfg, "mqtt|passfile", conf_valid_mqtt_passfile);

	switch (cfg_parse(cfg, args->conf_file)) {
	case CFG_FILE_ERROR:
		log_warn("Read configuration file %s failed (%d) %s\n" \
			 "Configuration file will be ignored\n",
			 args->conf_file, errno, strerror(errno));
		ret = -ENOENT;
		goto out;
	case CFG_PARSE_ERROR:
		log_err("Parse configuration file %s failed\n", args->conf_file);
		ret = -EINVAL;
		goto out;
	}

	if (cfg_getint(cfg, "loglevel") > 0)
		max_loglevel = cfg_getint(cfg, "loglevel");

	if (cfg_size(cfg, "mqtt") >= 0)
		cfg_mqtt = cfg_getnsec(cfg, "mqtt", 0);

	if (cfg_mqtt) {
		args->mqtt.host = strdup(cfg_getstr(cfg_mqtt, "host"));
		args->mqtt.port = (uint16_t)cfg_getint(cfg_mqtt, "port");
		args->mqtt.user = strdup(cfg_getstr(cfg_mqtt, "user"));
		args->mqtt.pass = strdup(cfg_getstr(cfg_mqtt, "password"));
		args->mqtt.passfile = strdup(cfg_getstr(cfg_mqtt, "passfile"));
	}

out:
	cfg_free(cfg);

	return ret;
}

static int get_mqtt_pass(const char *filename, const char **pass)
{
	int ret = 0;

	//TODO: use gpg to decrypt password from file

	return ret;
}

static int cmdline_handler(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	struct arguments args;
	int c;
	int long_optind = 0;

	args_set_default(argc, argv, &args);

	struct option long_options[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'v' },
		{ "loglevel", required_argument, NULL, 0 },
		{ "no-daemonize", no_argument, NULL, 'd' },
		{ "mod-path", required_argument, NULL, 'm' },
		{ "pidfile", required_argument, NULL, 'p' },
		{ "conffile", required_argument, NULL, 'c' },
		{ NULL, 0, NULL, 0 },
	};

	static const char *short_options = "hvdp:m:f:c:";

	while (0 <= (c = getopt_long(argc, argv, short_options, long_options, &long_optind))) {
		switch (c) {
		case 0:
			if (strcmp(long_options[long_optind].name, "loglevel") == 0) {
				int loglevel = atoi(optarg);
				if (loglevel >= LOGLEVEL_INFO && loglevel <= LOGLEVEL_DEBUG)
					max_loglevel = loglevel;
			}
			break;
		case 'h':
			usage(args.app_name);
			exit(EXIT_SUCCESS);
		case 'v':
			pr_version();
			exit(EXIT_SUCCESS);
		case 'd':
			args.daemonize = false;
			break;
		case 'p':
			args.pidfile = optarg;
			break;
		case 'm':
			args.moddir = optarg;
			break;
		case 'c':
			args.conf_file = optarg;
			break;
		case '?':
			break;
		default:
			log_err("getopt returned character code %d", c);
			break;
		}
	}

	if (optind < argc) {
		log_err("invalid arguments");
		usage(args.app_name);
		exit(EXIT_FAILURE);
	}

	if (!args.pidfile)
		args.pidfile = PIDFILE;

	if (!args.moddir)
		args.moddir = DEFAULT_MODULE_PATH;

	if (!args.conf_file)
		args.conf_file = DEFAULT_CONF_PATH;

	if (args.daemonize) {
		ret = daemon(0, 1);
		if (ret < 0) {
			log_err("daemonizing failed (%d) %s", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	create_pidfile(args.pidfile);

	ret = conf_set_args_from_conf_file(&args);
	if (ret == EINVAL)
		exit(EXIT_FAILURE);

	if (args.mqtt.passfile) {
		ret = get_mqtt_pass(args.mqtt.passfile, &args.mqtt.pass);
		if (ret < 0) {
			log_err("couldn't get mqtt password from %s (%d) %s",
				args.mqtt.passfile, ret, strerror(ret));
			exit(EXIT_FAILURE);
		}
	}

	log_info("initialization done");
	log_dbg("app name: %s PID: %d", args.app_name, getpid());

	ret = gpioex_init();
	if (ret)
		exit(EXIT_FAILURE);

	ret = run(&args);

	args_free(&args);

	return ret;
}

static void sig_handler(int signo)
{
	switch (signo) {
	case SIGTERM:
		mqtt_quit();
		log_dbg("Exit gardenctl");
		break;
	}
}

static int init_signal(void)
{
	int ret = EXIT_SUCCESS;

	if (signal(SIGTERM, sig_handler) == SIG_ERR) {
		log_err("catch signal SIGTERM failed (%d) %s", errno, strerror(errno));
		ret = EXIT_FAILURE;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	if (init_signal())
		return EXIT_FAILURE;

	return cmdline_handler(argc, argv);
}
