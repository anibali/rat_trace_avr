#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/power.h>
#include <avr/sleep.h>

#include "util.h"
#include "uart.h"
#include "softserial.h"
#include "wifi.h"
#include "adc.h"
#include "pin.h"
#include "rtc.h"
#include "report.h"
#include "clock.h"

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
  rtc_init();
  wifi_init(&uart_output, &uart_input, uart_available);
  report_init();

  printf("Compiled at: %s, %s\n", __TIME__, __DATE__);
  _delay_ms(100); // Give debug message time to send

  wifi_connect();

  clock_resync();

  adc_init();

  wifi_disable();
}

uint16_t vbat_read() {
  pin_digital_write(Pin_Battery_Test_Enable, Logic_High);
  _delay_ms(50);
  uint16_t vbat_value = adc_read(6);
  pin_digital_write(Pin_Battery_Test_Enable, Logic_Low);

  return vbat_value;
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

      //report_send();
      report_new();
    }

    distance_sum += ir_read();

    ++iterations;

    if(iterations >= max_iterations) {
      uint16_t distance_avg = distance_sum / iterations;
      printf("Avg distance: %u\n", distance_avg);

      if(true || distance_avg < 300) {
        if(!msg_waiting) {
          wifi_enable();
          msg_waiting = true;
        }
      }

      iterations = 0;
      distance_sum = 0;

      int16_t vbat = 4000 + ((vbat_read() - 217l) * 2000l) / (327 - 217);
      printf("Vbat = %d mV\n", vbat);
      report_add_battery_level_chunk(vbat);

      // uint32_t time_secs;
      // clock_get_time(&time_secs);
      // printf("Time = %lu\n", time_secs);
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
