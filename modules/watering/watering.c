#include <stdlib.h>
#include <config.h>
#include <garden_module.h>
#include <logging.h>

static int gm_init(void)
{
	int ret = 0;

	return ret;
}

struct garden_module *get_garden_module(void)
{
	struct garden_module *module = malloc(sizeof(*module));

	if (module) {
		module->init = gm_init;
	}

	return module;
}
