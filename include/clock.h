#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <stdint.h>

#define DAYS_TO_SECS 86400

void clock_resync();
void clock_set_base_time(uint32_t secs);
uint32_t clock_get_base_time();
uint32_t clock_get_time();

#endif
