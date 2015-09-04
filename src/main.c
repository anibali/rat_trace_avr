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

uint32_t base_time = 0;

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

static void resync_time() {
  Wifi_Error error;
  wifi_request_ntp(&base_time, &error);
  rtc_write_seconds(0);

  // TODO: Handle error
  // TODO: Adjust base time to compensate for processing time?
  printf("Base time: %lu, Error: %d\n", base_time, error);
}

void init() {
  pin_register_all();

  pin_set_direction(Pin_Debug_Out, Direction_Output);

  pin_set_direction(Pin_IR_Enable, Direction_Output);
  pin_set_direction(Pin_Status_LED, Direction_Output);
  pin_digital_write(Pin_Status_LED, Logic_High);

  pin_set_direction(Pin_Battery_Test_Enable, Direction_Output);

  sleep_init();
  uart_init();
  rtc_init();
  wifi_init(&uart_output, &uart_input, uart_available);

  // Use software serial for stdout and stdin (debugging purposes)
  softserial_init();
  stdout = &softserial_output;
  stdin  = &softserial_input;

  // Enable interrupts
  sei();

  printf("Compiled at: %s, %s\n", __TIME__, __DATE__);
  _delay_ms(100); // Give debug message time to send

  //uint32_t id[2];
  //rtc_read_id((uint64_t*)id);
  //printf("ID = %lx %lx\n", id[1], id[0]);

  wifi_connect();

  resync_time();

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

      int16_t vbat = 4000 + ((vbat_read() - 217l) * 2000l) / (327 - 217);
      printf("Vbat = %d mV\n", vbat);

      uint32_t rtc_time = rtc_read_seconds();
      printf("Time = %lu\n", base_time + rtc_time);
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
