#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

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

#ifdef _DEBUG
volatile uint16_t millis = 0;
ISR(TIMER2_COMPA_vect) {
  ++millis;
}
#endif

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

  // Prescale of 32, clear count on compare match
  TCCR2A = 0;
  TCCR2B = _BV(WGM21) | _BV(CS21) | _BV(CS20);
  // Set compare value for every ms
  OCR2A = F_CPU / (1000 * 32) - 1;
  // Enable Timer 2 compare interrupt
  TIMSK2 |= _BV(OCIE2A);

  printf("\n\nCompiled at: %s, %s\n", __TIME__, __DATE__);
#endif

  sleep_init();
  uart_init();
  i2c_init();
  rtc_init();
  wifi_init(&uart_output, &uart_input, uart_available);

  wifi_connect();

  check_resync();

  adc_init();
  proximity_init();
  report_init();

  wifi_disable();
}

static void run() {
  bool msg_waiting = false;
  const uint8_t max_iterations = 16;
  uint8_t iterations = 0;
  uint16_t distance_avg = 0;

  bool unreported_open = false;
  uint32_t opened_time = 0;

  while(1) {
    ++iterations;

#ifdef _DEBUG
    printf("Iteration %2d/%2d\n", iterations, max_iterations);

    ATOMIC_BLOCK(ATOMIC_FORCEON) {
      millis = 0;
    }
#endif

    bool do_send = msg_waiting && wifi_is_connected();

    if(do_send) {
      msg_waiting = false;

      report_send();
      report_new();
    }

    uint16_t proximity = proximity_measure_average(5);
    // EMA with alpha = 0.75
    distance_avg = (proximity - (proximity >> 2)) + (distance_avg >> 2);

    if(!unreported_open) {
      uint16_t als = als_measure();

      if(als > 115) {
        if(opened_time == 0) {
          opened_time = clock_get_time();
          unreported_open = true;
        }
      } else {
        opened_time = 0;
      }
    }

#ifdef _DEBUG
    pin_digital_write(Pin_Status_LED, !unreported_open);
#endif

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
        wifi_enable();
        msg_waiting = true;
      }

      uint16_t vbat = vbat_measure();
      printf("Vbat = %d mV\n", vbat);
      report_add_battery_level_chunk(vbat);

      if(unreported_open) {
        printf("Trap opened at %d\n", opened_time);
        report_add_trap_opened_chunk(opened_time);
        unreported_open = false;
      }

      iterations = 0;
    }

    if(do_send) {
      wifi_wait_for_send();
      //check_resync();
      wifi_disable();
    }

    // Give GPIOs time to change, etc
    //_delay_ms(200);

#ifdef _DEBUG
    printf("Processed for %d ms\n", millis);
#endif

    // Sleep
    sleep_now();
  }
}

int main() {
  init();
  run();

  return 0;
}
