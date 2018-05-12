#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "logging.h"

#define MAX_FMT_SIZE 1000

__attribute__((visibility("hidden"))) enum loglevel max_loglevel = LOGLEVEL_DEBUG;

static int log_prefix(FILE *logfile, enum loglevel level)
{
	int ret = 0;
	const char *prefix = NULL;

	switch(level) {
	case LOGLEVEL_INFO:
		prefix = "-I- ";
		break;
	case LOGLEVEL_ERROR:
		prefix = "-E- ";
		break;
	case LOGLEVEL_WARNING:
		prefix = "-W- ";
		break;
	case LOGLEVEL_DEBUG:
	default:
		prefix = "-D- ";
		break;
	}

	if (prefix)
		fprintf(logfile, "%s", prefix);

	return ret;
}

__attribute__((visibility("hidden"))) int logging(enum loglevel level, const char *fmt, ...)
{
	int ret = 0;
	va_list ap;
	FILE *logfile = stderr;
	size_t fmt_size = strnlen(fmt, MAX_FMT_SIZE);

	if (level > max_loglevel)
		goto out;

	if (fmt_size > 0 && fmt[fmt_size] != '\0') {
		fprintf(stderr, "-E- logging message to long\n");
		ret = -EINVAL;
		goto out;
	}

	ret = log_prefix(logfile, level);
	if (ret < 0)
		goto out;

	va_start(ap, fmt);
	ret = vfprintf(logfile, fmt, ap);
	va_end(ap);

	if (ret < 0)
		goto out;

	if (fmt_size > 0 && fmt[fmt_size - 1] != '\n')
		ret = fprintf(logfile, "\n");

out:
	return ret;
}
