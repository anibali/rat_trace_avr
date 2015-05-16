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
  _delay_ms(100); // Give debug message time to send

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

  wifi_disable();

  bool msg_waiting = false;

  int loop_count = 0;
  int breaks = 0;

  while(1) {
    uint16_t ir_value = adc_read(0);

    if(msg_waiting && wifi_is_connected()) {
      msg_waiting = false;
      // TODO: Reduce delay here by checking for successful send later
      wifi_send("1234MISSING");
      wifi_disable();
    }

    if(ir_value > 200) {
      ++breaks;
    }

    if(loop_count < 19) {
      ++loop_count;
    } else {
      loop_count = 0;

      if(breaks < 15) {
        if(!msg_waiting) {
          wifi_enable();
          msg_waiting = true;
          // Give GPIO time to change
          _delay_ms(500);
        }
      }

      breaks = 0;
    }

    sleep_now();
  }

  return 0;
}
