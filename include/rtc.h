#ifndef __RTC_H__
#define __RTC_H__

#include <stdint.h>

void rtc_init();
void rtc_read_id(uint64_t *id);
uint32_t rtc_read_seconds();
void rtc_write_seconds(uint32_t seconds);
uint8_t rtc_read_status();
void rtc_write_status(uint8_t status);
uint8_t rtc_read_control();
void rtc_write_control(uint8_t control);

#endif
