#include "uart.h"

#include <avr/io.h>
#include <util/delay.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef BAUD
#define BAUD 9600
#endif

#include <util/setbaud.h>

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

void uart_init() {
  // Assign values calculated by setbaud.h
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

#if USE_2X
  UCSR0A |= _BV(U2X0);
#else
  UCSR0A &= ~(_BV(U2X0));
#endif

  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data, 1 stop bit
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);   // Enable RX and TX

  // Use UART for stdin and stdout
  stdout = &uart_output;
  stdin  = &uart_input;
}

void uart_putchar(char c, FILE *stream) {
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
}

char uart_getchar(FILE *stream) {
  loop_until_bit_is_set(UCSR0A, RXC0);
  return UDR0;
}

int uart_getchar_timeout(FILE *stream, char *c) {
  int char_received = 0;
  int tries = 0;
  do {
    if(UCSR0A & _BV(RXC0)) {
      *c = UDR0;
      char_received = 1;
      break;
    }
    ++tries;
    _delay_ms(1);
  } while(tries < 1000);

  return char_received;
}
