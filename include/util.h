#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>
#include <avr/power.h>
#include <avr/sleep.h>

#define ARRAYSIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

uint32_t swap_endian(uint32_t val);

void sleep_init();
void sleep_now();

#endif
