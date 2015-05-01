#include "softserial.h"

#include "util.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>

#define SOFTSERIAL_BAUD 9600

static char tx_state = 8;
static volatile char tx_start = 0;
static volatile char tx_busy = 0;
static char tx_char;

static char rx_count;
static char rx_char;
#define rx_buffer_len 256
static char rx_buffer[rx_buffer_len];
static volatile int rx_buffer_pos = 0;
static int rx_buffer_read_pos = 0;

ISR(TIMER1_COMPA_vect) {
  char output;

  // Change state
  if(tx_busy) {
    if(tx_state < 9) {
      ++tx_state;
    } else {
      tx_state = 0;
    }
  } else if(tx_start) {
    tx_busy = 1;
    tx_state = 9;
    tx_start = 0;
  }

  // Determine output for current state
  switch(tx_state) {
  // Data bits
  case 0: case 1: case 2: case 3:
  case 4: case 5: case 6: case 7:
    output = tx_char & (1 << tx_state);
    break;
  // Stop bit
  case 8:
    output = 1;
    tx_busy = 0;
    break;
  // Start bit
  case 9:
    output = 0;
    break;
  }

  // Write to TX digital output
  if(output != 0) {
    PORTB |= _BV(PORTB5);
  } else {
    PORTB &= ~_BV(PORTB5);
  }
}

ISR(TIMER0_COMPA_vect) {
  if(rx_count % 2 == 0) {
    char rx_bit = rx_count / 2;

    if(rx_bit == 0) { // Start bit
      rx_char = 0;
    } else if(rx_bit == 9) { // Stop bit
      rx_buffer[rx_buffer_pos] = rx_char;
      rx_buffer_pos = (rx_buffer_pos + 1) % rx_buffer_len;

      // Disable this interrupt
      TIMSK0 &= ~_BV(OCIE0A);
      // Enable external RX interrupt
      PCMSK0 |= _BV(PCINT0);
    } else { // Data bit
      if(PINB & _BV(PINB0)) {
        rx_char |= _BV(rx_bit - 1);
      }
    }
  }

  ++rx_count;
}

ISR(PCINT0_vect) {
  // Note: If using multiple PCINT interrupts we need to store/compare prev
  // state to determine which pin changed.

  if((PINB & _BV(PINB0)) == 0) { // Falling edge
    // TODO: Start timer for 1/2 period of UART, sample on even counts (start
    // at 0, first read is start bit). Disable this interrupt in the meantime

    // Disable this interrupt
    PCMSK0 &= ~_BV(PCINT0);

    // Start timer at double frequency of UART
    rx_count = 0;
    TCNT0 = 0;
    TIFR0 |= _BV(OCIE0A);
    TIMSK0 |= _BV(OCIE0A);
  }
}

// Transmit a single character using software serial (blocking)
void softserial_putc(char c) {
  tx_char = c;
  tx_start = 1;
  while(tx_start || tx_busy);
}

// Transmit a null-terminated string using software serial (blocking)
void softserial_puts(const char *str) {
  for(int i = 0; str[i] != '\0'; ++i) {
    softserial_putc(str[i]);
  }
}

int softserial_available() {
  if(rx_buffer_read_pos > rx_buffer_pos) {
    return (rx_buffer_len + rx_buffer_pos) - rx_buffer_read_pos;
  } else {
    return rx_buffer_pos - rx_buffer_read_pos;
  }
}

char softserial_getc() {
  // Wait until there is something to read
  while(rx_buffer_read_pos == rx_buffer_pos);

  char c = rx_buffer[rx_buffer_read_pos];
  rx_buffer_read_pos = (rx_buffer_read_pos + 1) % rx_buffer_len;
  return c;
}

/**
 * Reads up to n-1 characters and puts them, along with a null terminator,
 * into s. Returns number of characters read.
 * IMPORTANT: s must be a pointer to at least n bytes of allocated memory!
 */
int softserial_getsn(char* s, int n) {
  int available = softserial_available();
  int n_chars = available < n ? available : n - 1;
  for(int i = 0; i < n_chars; ++i) {
    s[i] = softserial_getc();
  }
  s[n_chars] = '\0';

  return n_chars;
}

void softserial_dump() {
  printf("--- DUMP ---\n");
  char str[rx_buffer_len];
  softserial_getsn(str, ARRAYSIZE(str));
  printf(str);
  printf("--- DUMPED ---\n");
}

void softserial_init() {
  //// TX

  DDRB |= _BV(DDB5); // Set as output

  // Prescale of 8, clear count on compare match
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);
  // Set compare value corresponding to baud rate
  OCR1A = F_CPU / (8UL * SOFTSERIAL_BAUD);
  // Enable Timer 1 compare interrupt
  TIMSK1 |= _BV(OCIE1A);

  //// RX

  DDRB &= ~_BV(DDB0); // Set as input
  PORTB |= _BV(PORTB0); // Enable pull-up
  PCICR |= _BV(PCIE0);
  PCMSK0 |= _BV(PCINT0);

  // Prescale of 8, clear count on compare match
  TCCR0A = _BV(WGM01);
  TCCR0B = _BV(CS01);
  // Set compare value corresponding to double baud rate
  OCR0A = F_CPU / (8UL * 2 * SOFTSERIAL_BAUD);
}
