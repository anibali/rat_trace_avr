#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/power.h>
#include <avr/sleep.h>

#include "util.h"
#include "uart.h"
#include "wifi.h"
#include "adc.h"

ISR(WDT_vect) {}

void sleep_init() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  MCUSR &= ~_BV(WDRF);
  WDTCSR |= _BV(WDCE) | _BV(WDE);
  WDTCSR = _BV(WDP2); // 0.25 secs
  WDTCSR |= _BV(WDIE);
}

void sleep_now() {
  sleep_enable();
  power_all_disable();

  sleep_mode();

  sleep_disable();
  power_all_enable();
}

void init() {
  uart_init();
  printf("Compiled at: %s, %s\n", __TIME__, __DATE__);

  // Enable interrupts
  sei();

  sleep_init();

  wifi_init();
  wifi_connect();
  wifi_send("0000STARTED");

  adc_init();

  DDRD |= _BV(DDD2);
  // Enable IR sensor
  PORTD |= _BV(PORTD2);
}

int main() {
  init();

  wifi_sleep();

  int loop_count = 0;
  int breaks = 0;

  while(1) {
    uint16_t ir_value = adc_read(0);
    if(ir_value > 200) {
      ++breaks;
    }

    if(loop_count < 19) {
      ++loop_count;
    } else {
      loop_count = 0;

      if(breaks < 15) {
        wifi_wake(); // Takes about 8 seconds
        wifi_send("1234MISSING");
        wifi_sleep();
      }

      breaks = 0;
    }

    sleep_now();
  }

  return 0;
}
