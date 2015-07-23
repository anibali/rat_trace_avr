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
#include "pin.h"

// TODO: Proper structs for sending reports
#pragma pack(push,1)
typedef struct {
  char identifier[4];
  uint8_t protocol_version;
  uint32_t trap_id;
  uint32_t send_time;
  uint8_t n_chunks;
} Test_Report;
#pragma pack(pop)

ISR(WDT_vect) {}

void sleep_init() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  MCUSR &= ~_BV(WDRF);
  WDTCSR |= _BV(WDCE) | _BV(WDE);
  //WDTCSR = _BV(WDP2); // 0.25 secs
  WDTCSR = _BV(WDP2) | _BV(WDP1); // 1.0 secs
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
  pin_init();

  uart_init();
  printf("Compiled at: %s, %s\n", __TIME__, __DATE__);
  _delay_ms(100); // Give debug message time to send

  // Enable interrupts
  sei();

  sleep_init();

  wifi_init();
  wifi_connect();

  Wifi_Error error;
  uint32_t time_val = wifi_request_time(&error);
  printf("Time val: %lu, Error: %d\n", time_val, error);

  adc_init();

  pin_set_direction(Pin_IR_Enable, Direction_Output);

  wifi_disable();
}

uint16_t ir_read() {
  /* Read from IR sensor */
  pin_digital_write(Pin_IR_Enable, Logic_High);
  _delay_ms(50);
  uint16_t ir_value = adc_read(0);
  pin_digital_write(Pin_IR_Enable, Logic_Low);
  _delay_ms(50);

  return ir_value;
}

int main() {
  init();

  bool msg_waiting = false;
  const uint8_t max_iterations = 16;
  uint8_t iterations = 0;
  uint16_t distance_sum = 0;

  while(1) {
    bool do_send = msg_waiting && wifi_is_connected();

    if(do_send) {
      msg_waiting = false;

      Test_Report report = {
        "RATR",
        1,
        42,
        100,
        0
      };

      wifi_sendn(&report, sizeof(report));
    }

    distance_sum += ir_read();

    ++iterations;

    if(iterations >= max_iterations) {
      uint16_t distance_avg = distance_sum / iterations;
      printf("Avg distance: %u\n", distance_avg);

      if(distance_avg < 300) {
        if(!msg_waiting) {
          wifi_enable();
          msg_waiting = true;
        }
      }

      iterations = 0;
      distance_sum = 0;
    }

    if(do_send) {
      wifi_wait_for_send();
      wifi_disable();
    }

    // Give GPIOs time to change, etc
    _delay_ms(500);
    // Sleep
    sleep_now();
  }

  return 0;
}
