#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#ifndef BAUD
#define BAUD 9600
#endif

#include <util/setbaud.h>

#define rx_buffer_len 512
static char rx_buffer[rx_buffer_len];
static volatile int rx_buffer_pos = 0;
static int rx_buffer_read_pos = 0;

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

#include "pin.h"

ISR(USART_RX_vect) {
  rx_buffer[rx_buffer_pos] = UDR0;
  rx_buffer_pos = (rx_buffer_pos + 1) % rx_buffer_len;
}

void uart_init() {
  // Assign values calculated by setbaud.h
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

  // Disable double transmission speed
  UCSR0A &= ~(_BV(U2X0));

  // 8-bit data, 1 stop bit
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
  // Enable RX (with interrupt) and TX
  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
}

void uart_putchar(char c, FILE *stream) {
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
}

char uart_getchar(FILE *stream) {
  // Wait until there is something to read
  while(rx_buffer_read_pos == rx_buffer_pos);

  char c = rx_buffer[rx_buffer_read_pos];
  rx_buffer_read_pos = (rx_buffer_read_pos + 1) % rx_buffer_len;
  return c;
}

int uart_available() {
  if(rx_buffer_read_pos > rx_buffer_pos) {
    return (rx_buffer_len + rx_buffer_pos) - rx_buffer_read_pos;
  } else {
    return rx_buffer_pos - rx_buffer_read_pos;
  }
}
