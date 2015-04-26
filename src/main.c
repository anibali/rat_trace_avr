#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"
#include "wifi.h"

int main() {
  uart_init();
  printf("Compiled at: %s, %s\n", __TIME__, __DATE__);

  // Enable interrupts
  sei();

  wifi_init();
  wifi_connect();

  while(1) {
    wifi_send("1234H5");

    wifi_sleep();
    _delay_ms(10000);
    wifi_wake();
  }

  return 0;
}
