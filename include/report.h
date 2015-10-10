#ifndef __REPORT_H__
#define __REPORT_H__

#include <stdint.h>

void report_init();
void report_send();
void report_new();
void report_add_battery_level_chunk(uint16_t level);
void report_add_bait_level_chunk(uint16_t bait_id, uint16_t level);
void report_add_trap_opened_chunk(uint32_t opened_time);

#endif
