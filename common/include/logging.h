#ifndef __LOGGING_H__
#define __LOGGING_H__

enum loglevel {
	LOGLEVEL_INFO,
	LOGLEVEL_ERROR,
	LOGLEVEL_WARNING,
	LOGLEVEL_DEBUG,
};

extern enum loglevel max_loglevel;

extern int logging(enum loglevel level, const char *fmt, ...);

#define log_info(__fmt, ...) logging(LOGLEVEL_INFO, (__fmt), ##__VA_ARGS__)
#define log_err(__fmt, ...) logging(LOGLEVEL_ERROR, (__fmt), ##__VA_ARGS__)
#define log_warn(__fmt, ...) logging(LOGLEVEL_WARNING, (__fmt), ##__VA_ARGS__)
#define log_dbg(__fmt, ...) logging(LOGLEVEL_DEBUG, (__fmt), ##__VA_ARGS__)

#endif /*__LOGGING_H__*/
