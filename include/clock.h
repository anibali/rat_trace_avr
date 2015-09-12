#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>

void clock_resync();
void clock_get_time(uint32_t *secs);

#endif
