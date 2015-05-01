#include "adc.h"

#include <avr/io.h>

void adc_init() {
  // Use AVCC as voltage ref
  ADMUX |= _BV(REFS0);
  // Set prescale of 128
  ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
}

uint16_t adc_read(uint8_t channel) {
  // Enable ADC
  ADCSRA |= _BV(ADEN);
  // Select channel
  ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
  // One-shot ADC conversion
  ADCSRA |= _BV(ADSC);
  // Wait for result
  loop_until_bit_is_clear(ADCSRA, ADSC);
  // Enable ADC
  ADCSRA &= ~_BV(ADEN);
  // Return result
  return ADC;
}
