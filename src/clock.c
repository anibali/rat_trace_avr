#include "clock.h"

#include <avr/eeprom.h>

#include "rtc.h"
#include "wifi.h"

// Time compensation for difference between time returned by NTP server
// and time when that value is written to local memory
#define RESYNC_COMPENSATION 2

static uint32_t base_time = 0;

void clock_resync() {
  printf("[CLOCK] Resyncing time...\n");

  Wifi_Error error = -1;
  int tries_left = 5;
  uint32_t new_base_time;

  do {
    wifi_request_ntp(&new_base_time, &error);
    --tries_left;
  } while(error != Wifi_Error_None && tries_left > 0);

  if(error == Wifi_Error_None) {
    rtc_write_seconds(0);
    clock_set_base_time(new_base_time + RESYNC_COMPENSATION);
    printf("[CLOCK] Resync successful\n");
  } else {
    printf("[CLOCK] Resync failed\n");
  }
}

inline uint32_t clock_get_base_time() {
  return base_time;
}

inline void clock_set_base_time(uint32_t secs) {
  base_time = secs;
}

uint32_t clock_get_time() {
  return rtc_read_seconds() + clock_get_base_time();
}

bool clock_should_resync() {
  uint32_t base_time = clock_get_base_time();
  return base_time == 0 || rtc_read_seconds() > 4 * DAYS_TO_SECS;
}
