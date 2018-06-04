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
#include <sys/queue.h>
#include <dlfcn.h>
#include <linux/limits.h>

#include "arguments.h"
#include "garden_module.h"
#include "mqtt.h"

LIST_HEAD(dl_modules, dl_module) dl_modules_head;
struct dl_modules *headp;
struct dl_module {
	void *dl_handler;
	struct garden_module *module;
	LIST_ENTRY(dl_module) dl_modules;
};

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

static int create_dl_module(const char *filename)
{
	int ret = 0;
	struct dl_module *module = malloc(sizeof(*module));
	const char* dl_err = NULL;

	if (module) {
		memset(module, 0, sizeof(*module));
		dlerror();
		module->dl_handler = dlopen(filename, RTLD_NOW);

		if ((dl_err = dlerror()) == NULL) {
			struct garden_module* (*get_garden_module)(void);
			get_garden_module = dlsym(module->dl_handler, "get_garden_module");

			if ((dl_err = dlerror()) == NULL) {
				module->module = get_garden_module();
				LIST_INSERT_HEAD(&dl_modules_head, module, dl_modules);
			} else {
				ret = 1;
				dlclose(module->dl_handler);
				free(module);
			}
		} else {
			log_err("dlopen for %s failed %s", filename, dl_err);
			ret = -EINVAL;
		}
	} else {
		log_err("Allocate memory for module failed.");
		ret = -ENOMEM;
	}

	return ret;
}

static int run(struct arguments *args)
{
	int ret = EXIT_SUCCESS;
	DIR *d = opendir(args->moddir);
	struct dirent *dir = NULL;
	struct dl_module *dlm;

	log_dbg("modules directory: %s", args->moddir);

	if (d == NULL) {
		log_err("Couldn't open %s directory.", args->moddir);
		ret = EXIT_FAILURE;
		goto out;
	}

	LIST_INIT(&dl_modules_head);

	while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_REG) {
			log_dbg("directory content: %s", dir->d_name);

			if (match_regex("libgarden_.*\\.so$", dir->d_name)) {
				char path[PATH_MAX];
				sprintf(path, "%s%s", args->moddir, dir->d_name);
				ret = create_dl_module(path);
				if (ret < 0)
					goto out_remove_dl_modules;
			}
		}
	}

	for(dlm = dl_modules_head.lh_first; dlm != NULL; dlm = dlm->dl_modules.le_next) {
		if (dlm->module->init)
			dlm->module->init();
	}

	ret = mqtt_run(args->mqtt_user, args->mqtt_pass);

out_remove_dl_modules:
	while(dl_modules_head.lh_first != NULL) {
		dlclose(dl_modules_head.lh_first->dl_handler);
		LIST_REMOVE(dl_modules_head.lh_first, dl_modules);
	}

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
