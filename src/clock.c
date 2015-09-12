#include "clock.h"

#include "rtc.h"
#include "wifi.h"

static uint32_t base_time = 0;

void clock_resync() {
  Wifi_Error error = -1;
  int tries_left = 5;

  do {
    wifi_request_ntp(&base_time, &error);
    --tries_left;
  } while(error != Wifi_Error_None && tries_left > 0);

  if(error == Wifi_Error_None) {
    rtc_write_seconds(0);
    // TODO: Handle errors
  } else {
    // TODO: Handle errors
  }
}

void clock_get_time(uint32_t *secs) {
  // TODO: Handle errors
  uint32_t rtc_time = rtc_read_seconds();
  *secs = base_time + rtc_time;
}
