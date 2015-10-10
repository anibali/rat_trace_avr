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
static uint16_t vbat_measure() {
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

#ifdef _DEBUG
  // Use software serial for stdout and stdin (debugging purposes)
  softserial_init();
  stdout = &softserial_output;
  stdin  = &softserial_input;

  printf("\n\nCompiled at: %s, %s\n", __TIME__, __DATE__);
#endif

  sleep_init();
  uart_init();
  i2c_init();
  rtc_init();
  //wifi_init(&uart_output, &uart_input, uart_available);

  //wifi_connect();

  //check_resync();

  adc_init();
  proximity_init();
  report_init();

  //wifi_disable();
}

static void run() {
  bool msg_waiting = false;
  const uint8_t max_iterations = 16;
  uint8_t iterations = 0;
  uint16_t distance_avg = 0;

  bool was_opened = false;
  uint32_t opened_time = 0;

  while(1) {
    ++iterations;

    printf("Iteration %2d/%2d\n", iterations, max_iterations);

    bool do_send = msg_waiting;// && wifi_is_connected();

    if(do_send) {
      msg_waiting = false;

      //report_send();
      report_new();
    }

    distance_avg += proximity_measure_average(9) / max_iterations;

    uint16_t als = als_measure();

    if(als > 50) {
      if(opened_time == 0) {
        opened_time = clock_get_time();
        was_opened = true;
      }
      pin_digital_write(Pin_Status_LED, Logic_Low);
    } else {
      if(was_opened == false) {
        opened_time = 0;
      }
      pin_digital_write(Pin_Status_LED, Logic_High);
    }

    if(iterations >= max_iterations) {
      int16_t percentage = 100 - (distance_avg - 44000u) / 150;
      if(distance_avg < 44000u) {
        percentage = 100;
      } else if(percentage < 0) {
        percentage = 0;
      }

      printf("Bait percentage: %u\n", percentage);
      report_add_bait_level_chunk(1, percentage);

      if(!msg_waiting) {
        //wifi_enable();
        msg_waiting = true;
      }

      uint16_t vbat = vbat_measure();
      printf("Vbat = %d mV\n", vbat);
      report_add_battery_level_chunk(vbat);

      if(was_opened) {
        printf("Trap opened at %d\n", opened_time);
        report_add_trap_opened_chunk(opened_time);
      }

      iterations = 0;
      distance_avg = 0;
      was_opened = false;
    }

    if(do_send) {
      //wifi_wait_for_send();
      //check_resync();
      //wifi_disable();
    }

    // Give GPIOs time to change, etc
    _delay_ms(200);
    // Sleep
    sleep_now();
  }
}

int main() {
  init();
  run();

  return 0;
}
