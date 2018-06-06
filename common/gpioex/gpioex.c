#include <gpioex.h>
#include <pthread.h>
#include <logging.h>
#include <string.h>
#include <linux/i2c-dev.h>

static pthread_mutex_t lock;

int gpioex_init(void)
{
	int ret = 0;

	ret = pthread_mutex_init(&lock, NULL);
	if (ret) {
		log_err("initiate mutex failed (%d) %s", ret, strerror(ret));
		goto out;
	}

out:
	return ret;
}

int gpioex_set(uint32_t gpio, int value)
{
	int ret = 0;

	log_dbg("gpioex: gpio: 0x%08X value: %d", gpio, value);

	pthread_mutex_lock(&lock);

	pthread_mutex_unlock(&lock);
	return ret;
}
