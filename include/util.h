#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>

#define ARRAYSIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

uint32_t swap_endian(uint32_t val);

#endif
