#include "softserial.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "util.h"
#include "pin.h"

#define SOFTSERIAL_BAUD 9600

FILE softserial_output = FDEV_SETUP_STREAM(softserial_putc, NULL, _FDEV_SETUP_WRITE);
FILE softserial_input = FDEV_SETUP_STREAM(NULL, softserial_getc, _FDEV_SETUP_READ);

static char tx_state = 8;
static volatile char tx_start = 0;
static volatile char tx_busy = 0;
static char tx_char;

static char rx_count;
static char rx_char;
#define rx_buffer_len 512
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

  pin_digital_write(Pin_Softserial_TX, output == 0 ? Logic_Low : Logic_High);
}

ISR(TIMER0_COMPA_vect) {
  if(rx_count % 2 == 0) {
    char rx_bit = rx_count / 2;

    if(rx_bit == 0) { // Start bit
      pin_digital_toggle(Pin_Debug_Out);
      rx_char = 0;
    } else if(rx_bit == 9) { // Stop bit
      pin_digital_toggle(Pin_Debug_Out);
      rx_buffer[rx_buffer_pos] = rx_char;
      rx_buffer_pos = (rx_buffer_pos + 1) % rx_buffer_len;

      // Disable this interrupt
      TIMSK0 &= ~_BV(OCIE0A);
      // Enable external RX interrupt
      pin_enable_interrupt(Pin_Softserial_RX);
    } else { // Data bit
      //pin_digital_toggle(Pin_Debug_Out);
      if(pin_digital_read(Pin_Softserial_RX) == Logic_High) {
        rx_char |= _BV(rx_bit - 1);
      }
    }
  }

  ++rx_count;
}

ISR(PCINT1_vect) {
  // NOTE: If using multiple PCINT interrupts we need to store/compare prev
  // state to determine which pin changed.

  if(pin_digital_read(Pin_Softserial_RX) == Logic_Low) { // Falling edge
    // Disable this interrupt
    pin_disable_interrupt(Pin_Softserial_RX);

    // Start timer at double frequency of UART
    rx_count = 0;
    TCNT0 = 0;
    TIFR0 |= _BV(OCIE0A);
    TIMSK0 |= _BV(OCIE0A);
  }
}

void softserial_init() {
  //// TX

  pin_set_direction(Pin_Softserial_TX, Direction_Output);

  // Prescale of 8, clear count on compare match
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);
  // Set compare value corresponding to baud rate
  OCR1A = F_CPU / (8UL * SOFTSERIAL_BAUD);
  // Enable Timer 1 compare interrupt
  TIMSK1 |= _BV(OCIE1A);

  //// RX

  pin_set_direction(Pin_Softserial_RX, Direction_Input);
  pin_digital_write(Pin_Softserial_RX, Logic_High); // Enable pull-up
  pin_enable_interrupt(Pin_Softserial_RX);

  // Prescale of 8, clear count on compare match
  TCCR0A = _BV(WGM01);
  TCCR0B = _BV(CS01);
  // Set compare value corresponding to double baud rate
  const int sample_clock_adjust = -3; // Twiddle factor tweaks sample rate
  OCR0A = F_CPU / (8UL * 2 * SOFTSERIAL_BAUD) + sample_clock_adjust;
}

// Transmit a single character using software serial (blocking)
void softserial_putc(char c) {
  tx_char = c;
  tx_start = 1;
  while(tx_start || tx_busy);
}

char softserial_getc() {
  // Wait until there is something to read
  while(rx_buffer_read_pos == rx_buffer_pos);

  char c = rx_buffer[rx_buffer_read_pos];
  rx_buffer_read_pos = (rx_buffer_read_pos + 1) % rx_buffer_len;
  return c;
}

int softserial_available() {
  if(rx_buffer_read_pos > rx_buffer_pos) {
    return (rx_buffer_len + rx_buffer_pos) - rx_buffer_read_pos;
  } else {
    return rx_buffer_pos - rx_buffer_read_pos;
  }
}

#if 0
// Transmit a null-terminated string using software serial (blocking)
void softserial_puts(const char *str) {
  for(int i = 0; str[i] != '\0'; ++i) {
    softserial_putc(str[i]);
  }
}

int softserial_printf(const char* format, ...) {
  // NOTE: This imposes a limit on max message size
  char buffer[128];

  va_list arglist;
  va_start(arglist, format);
  int n_chars = vsnprintf(buffer, ARRAYSIZE(buffer), format, arglist);
  va_end(arglist);

  softserial_puts(buffer);

  return n_chars;
}

void softserial_clear_buffer() {
  rx_buffer_read_pos = rx_buffer_pos;
}

bool softserial_getc_timeout(char *c, const int timeout_ms) {
  for(int i = 0; i < timeout_ms / 10; ++i) {
    if(rx_buffer_read_pos == rx_buffer_pos) {
      _delay_ms(10);
    } else {
      *c = rx_buffer[rx_buffer_read_pos];
      rx_buffer_read_pos = (rx_buffer_read_pos + 1) % rx_buffer_len;
      return true;
    }
  }

  return false;
}

// TODO: This function is currently blocking. It would be wiser to use
// some sort of timeout
int softserial_readline(char *s, int n) {
  int i;
  char c = 0;

  for(i = 0; i < (n - 1) && c != '\n'; ++i) {
    c = softserial_getc();
    s[i] = c;
  }
  s[i] = '\0';

  return i;
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
#endif
