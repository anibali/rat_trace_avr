#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "util.h"
#include "uart.h"
#include "softserial.h"
#include "wifi.h"
#include "adc.h"
#include "pin.h"
#include "rtc.h"
#include "report.h"
#include "clock.h"
#include "i2c.h"
#include "proximity.h"

EMPTY_INTERRUPT(WDT_vect);

/**
 * Check how long it's been since the last resync, and perform a resync
 * if it's been too long.
 */
static void check_resync() {
  uint32_t base_time = clock_get_base_time();

  if(base_time == 0 || clock_get_time() - base_time > 1 * DAYS_TO_SECS) {
    clock_resync();
  }
}

void init() {
  pin_register_all();

  pin_set_direction(Pin_Debug_Out, Direction_Output);

  pin_set_direction(Pin_IR_Enable, Direction_Output);
  pin_set_direction(Pin_Status_LED, Direction_Output);
  pin_digital_write(Pin_Status_LED, Logic_High);

  pin_set_direction(Pin_Battery_Test_Enable, Direction_Output);

  // Enable interrupts
  sei();

  // Use software serial for stdout and stdin (debugging purposes)
  softserial_init();
  stdout = &softserial_output;
  stdin  = &softserial_input;

  sleep_init();
  uart_init();
  i2c_init();
  //rtc_init();
  proximity_init();
  wifi_init(&uart_output, &uart_input, uart_available);
  //report_init();

  printf("Compiled at: %s, %s\n", __TIME__, __DATE__);
  _delay_ms(100); // Give debug message time to send

  //wifi_connect();

  //check_resync();

  //adc_init();

  //wifi_disable();

  uint16_t proximity;

  while(1) {
    proximity = proximity_measure();
    printf("%u\n", proximity);

    pin_digital_write(Pin_Status_LED,
      proximity > 3000 ? Logic_Low : Logic_High);

    _delay_ms(100);
  }
}

uint16_t vbat_measure() {
  pin_digital_write(Pin_Battery_Test_Enable, Logic_High);
  _delay_ms(50);
  uint16_t vbat_value = adc_read(6);
  pin_digital_write(Pin_Battery_Test_Enable, Logic_Low);

  return vbat_value;
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

      report_send();
      report_new();
    }

    distance_sum += 0;

    ++iterations;

    uint32_t secs = clock_get_time();
    printf("Secs: %ld\n", secs);

    if(iterations >= max_iterations) {
      uint16_t distance_avg = distance_sum / iterations;
      printf("Avg distance: %u\n", distance_avg);

      // if(distance_avg < 300) {
      //   if(!msg_waiting) {
      //     wifi_enable();
      //     msg_waiting = true;
      //   }
      // }

      iterations = 0;
      distance_sum = 0;

      int16_t vbat = 4000 + ((vbat_measure() - 217l) * 2000l) / (327 - 217);
      printf("Vbat = %d mV\n", vbat);
      report_add_battery_level_chunk(vbat);
    }

    if(do_send) {
      wifi_wait_for_send();
      check_resync();
      wifi_disable();
    }

    // Give GPIOs time to change, etc
    _delay_ms(500);
    // Sleep
    sleep_now();
  }

  return 0;
}
