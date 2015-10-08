#include "util.h"

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

void sort(uint16_t *array, int length) {
  // Bubble sort
  for(int i = 0; i < length - 1; ++i) {
    for(int j = 0; j < length - (i + 1); ++j) {
      if(array[j] > array[j+1]) {
        // Swap values
        array[j]    = array[j] ^ array[j+1];
        array[j+1]  = array[j] ^ array[j+1];
        array[j]    = array[j] ^ array[j+1];
      }
    }
  }
}

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
