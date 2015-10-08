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

/**
 * Read the battery voltage. Returns voltage in millivolts.
 */
uint16_t vbat_measure() {
  pin_digital_write(Pin_Battery_Test_Enable, Logic_High);
  _delay_ms(5);
  uint16_t adc_val = adc_read(6);
  pin_digital_write(Pin_Battery_Test_Enable, Logic_Low);

  // Vbat (mA) = 6.4516129 * adc_val
  uint16_t vbat = (adc_val * UINT32_C(13213)) >> 11;

  return vbat;
}

static void init() {
  pin_register_all();

  pin_set_direction(Pin_Debug_Out, Direction_Output);

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
  rtc_init();
  proximity_init();
  wifi_init(&uart_output, &uart_input, uart_available);
  report_init();

  printf("\n\nCompiled at: %s, %s\n", __TIME__, __DATE__);
  _delay_ms(100); // Give debug message time to send

  //wifi_connect();

  //check_resync();

  adc_init();

  //wifi_disable();
}

static void run() {
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

      uint16_t vbat = vbat_measure();
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
}

int main() {
  init();
  //run();

  ////
  uint16_t proximity;
  uint16_t count = 0;

  while(1) {
    proximity = proximity_measure();

    if(++count > 9) {
      uint32_t secs = rtc_read_seconds();
      uint16_t vbat = vbat_measure();
      printf("Time: %lu, Vbat: %u, Prox: %u\n",
        secs, vbat, proximity);
      count = 0;
    }

    pin_digital_write(Pin_Status_LED,
      proximity < 30000 ? Logic_Low : Logic_High);

    _delay_ms(50);
  }
  ////

  return 0;
}
