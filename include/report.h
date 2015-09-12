#ifndef __REPORT_H__
#define __REPORT_H__

#include <stdint.h>

void report_init();
void report_send();
void report_new();
void report_add_battery_level_chunk(uint16_t level);

#endif
