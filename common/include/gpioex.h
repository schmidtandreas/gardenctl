#ifndef __GPIOEX_H__
#define __GPIOEX_H__

#include <stdint.h>

#define GPIOEX_TAP		0x00000001
#define GPIOEX_YARD_LEFT	0x00000002
#define GPIOEX_YARD_RIGHT	0x00000004
#define GPIOEX_YARD_FRONT	0x00000008
#define GPIOEX_YARD_BACK	0x00000010
#define GPIOEX_DROPPIPE_PUMP	0x00000020
#define GPIOEX_BARREL		0x00000040
#define GPIOEX_LIGHT_TREE	0x00000080
#define GPIOEX_LIGHT_HOUSE	0x00000100

int gpioex_init(void);
int gpioex_set(uint32_t gpio, int value);

#endif /*__GPIOEX_H__*/
