#include <garden_common.h>
#include <errno.h>
#include <string.h>

int payload_on_off_to_int(const char *payload, size_t len)
{
	int ret = -EINVAL;

	if (len >= 2 && len <= 3) {
		if (strcmp("on", payload) == 0)
			ret = 1;
		else if (strcmp("off", payload) == 0)
			ret = 0;
	}

	return ret;
}
