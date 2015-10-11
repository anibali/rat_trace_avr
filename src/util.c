#include "util.h"

#include <avr/wdt.h>
#include <avr/interrupt.h>

uint32_t swap_endian(uint32_t val) {
  return
    ((val & 0xFF      ) << 24 ) |
    ((val & 0xFF00    ) << 8  ) |
    ((val & 0xFF0000  ) >> 8  ) |
    ((val & 0xFF000000) >> 24 );
}

uint16_t sqrt_u32(uint32_t x) {
  uint16_t result = 0;
  uint16_t add = UINT16_C(0x8000);

  for(int i = 0; i < 16; ++i) {
    uint16_t tmp = result | add;
    uint32_t g_squared = (uint32_t)tmp * tmp;
    if(x >= g_squared) result = tmp;
    add >>= 1;
  }

  return result;
}

/**
 * Sorts an array of integers in ascending order.
 * Current implementation is insertion sort
 */
void sort(uint16_t *array, int length) {
  for(int i = 1; i < length; ++i) {
    int j = i;
    while(j > 0 && array[j - 1] > array[j]) {
      uint16_t tmp = array[j];
      array[j] = array[j - 1];
      array[j - 1] = tmp;
      --j;
    }
  }
}

ISR(WDT_vect) {
  // Disable WDT
  wdt_disable();
}

void sleep_init() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void sleep_now() {
  // Set WDT to wake us up after 1 second
  wdt_enable(WDTO_1S);
  WDTCSR |= _BV(WDIE);

  // Disable all modules
  power_all_disable();

  cli();
  sleep_enable();
  sei();
  sleep_cpu();
  sleep_disable();

  // Enable all modules after wake
  power_all_enable();
}
