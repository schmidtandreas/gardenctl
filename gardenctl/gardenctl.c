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
#include <gpioex.h>

#include "arguments.h"
#include "dl_module.h"
#include "mqtt.h"

static int match_regex(const char *pattern, const char *string) {
	int ret = 0;
	regex_t re;

	ret = regcomp(&re, pattern, REG_ICASE | REG_EXTENDED);
	if (ret < 0)
		return 0;

	ret = regexec(&re, string, (size_t) 0, NULL, 0);
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

	ret = mqtt_run(&dlm_head, args->mqtt_user, args->mqtt_pass);

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
	args->app_name = argv[0];
	args->pidfile = NULL;
	args->moddir = NULL;
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
	"  -p, --pidfile FILE        specify path for pid file\n"
	"  -m, --mod-path PATH       specify path for modules\n"
	"  -u, --mqtt_user USER      mqtt username\n"
	"  -s, --mqtt_pass PASS      mqtt password\n"
	"  -f, --mqtt_passfile FILE  mqtt password file\n"
	, app_name, max_loglevel);
}

static void pr_version(void)
{
	fprintf(stdout, "gardenctl (%s) %s\n", PACKAGE, VERSION);
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
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{"loglevel", required_argument, NULL, 0},
		{"mod-path", required_argument, NULL, 'm'},
		{"mqtt_user", required_argument, NULL, 'u'},
		{"mqtt_pass", required_argument, NULL, 's'},
		{"mqtt_passfile", required_argument, NULL, 'f'},
		{NULL, 0, NULL, 0},
	};

	static const char *short_options = "hvp:m:u:s:f:";

	while(0 <= (c = getopt_long(argc, argv, short_options, long_options, &long_optind))) {
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
		case 'p':
			args.pidfile = optarg;
			break;
		case 'm':
			args.moddir = optarg;
			break;
		case 'u':
			args.mqtt_user = optarg;
			break;
		case 's':
			args.mqtt_pass = optarg;
			break;
		case 'f':
			args.mqtt_passfile = optarg;
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

	ret = daemon(0, 1);
	if (ret < 0) {
		log_err("daemonizing failed (%d) %s", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (args.mqtt_passfile) {
		ret = get_mqtt_pass(args.mqtt_passfile, &args.mqtt_pass);
		if (ret < 0) {
			log_err("couldn't get mqtt password from %s (%d) %s",
					args.mqtt_passfile, ret, strerror(ret));
			exit(EXIT_FAILURE);
		}
	}

	log_info("initialization done");
	create_pidfile(args.pidfile);
	log_dbg("app name: %s PID: %d", args.app_name, getpid());

	ret = gpioex_init();
	if (ret)
		exit(EXIT_FAILURE);

	ret = run(&args);

	return ret;
}

static void sig_handler(int signo)
{
	switch(signo) {
	case SIGTERM:
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
