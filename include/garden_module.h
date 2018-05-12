#ifndef __GARDEN_MODULE_H__
#define __GARDEN_MODULE_H__

struct garden_module {
	int (*init)(void);
};

#endif /*__GARDEN_MODULE_H__*/
