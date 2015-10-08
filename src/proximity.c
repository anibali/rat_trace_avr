#include "proximity.h"

#include <avr/io.h>
#include <util/delay.h>

#include "i2c.h"
#include "util.h"

#define VCNL4000_Address 0x26

typedef enum {
  VCNL4000_Reg_Command = 0x80,
  VCNL4000_Reg_Product_ID,
  VCNL4000_Reg_LED_Current = 0x83,
  VCNL4000_Reg_Ambient_Param,
  VCNL4000_Reg_Ambient_Result_High,
  VCNL4000_Reg_Ambient_Result_Low,
  VCNL4000_Reg_Proximity_Result_High,
  VCNL4000_Reg_Proximity_Result_Low,
  VCNL4000_Reg_Proximity_Frequency,
  VCNL4000_Reg_Proximity_Timing,
} VCNL4000_Reg;

#define PROX_DATA_RDY 5
#define PROX_OD       3

static inline uint8_t vcnl_read(uint8_t reg) {
  uint8_t data;
  i2c_read_register(&data, 1, VCNL4000_Address, reg, true, NULL);
  return data;
}

static inline void vcnl_write(uint8_t reg, uint8_t val) {
  i2c_write_register(&val, 1, VCNL4000_Address, reg, false, NULL);
}

static uint16_t linearize(uint16_t raw_value) {
  // TODO: Tune these values
  const uint32_t m = 1266730;
  const uint16_t c = 2000;

  uint32_t sqrt_m = sqrt_u32(m * 40 * 40); // Can be precalculated
  uint32_t raw_take_c = raw_value - c;

  uint16_t microns =
    (sqrt_m * sqrt_u32(raw_take_c * 25 * 25)) / raw_take_c;

  return microns;
}

void proximity_init() {
  // Set IR LED current (current = value * 10mA)
  vcnl_write(VCNL4000_Reg_LED_Current, 15);
}

/**
 * Take a proximity measurement. Returns distance in micrometres.
 */
uint16_t proximity_measure() {
  uint8_t data;
  uint16_t proximity;

  vcnl_write(VCNL4000_Reg_Command, _BV(PROX_OD));

  do {
    _delay_ms(1);
  } while(!(vcnl_read(VCNL4000_Reg_Command) & _BV(PROX_DATA_RDY)));

  proximity = vcnl_read(VCNL4000_Reg_Proximity_Result_High);
  proximity <<= 8;
  proximity |= vcnl_read(VCNL4000_Reg_Proximity_Result_Low);

  return linearize(proximity);
}
