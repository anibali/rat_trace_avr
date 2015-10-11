#include "proximity.h"

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

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

#define MAX_PROX_SAMPLES 31

#define ALS_DATA_RDY  6
#define PROX_DATA_RDY 5
#define ALS_OD        4
#define PROX_OD       3

static inline uint8_t vcnl_read(uint8_t reg) {
  uint8_t data;
  i2c_read_register(&data, 1, VCNL4000_Address, reg, true, NULL);
  return data;
}

static inline void vcnl_write(uint8_t reg, uint8_t val) {
  i2c_write_register(&val, 1, VCNL4000_Address, reg, false, NULL);
}

// Value read when no obstacle is in front of sensor
#define PROX_C (UINT16_C(2530))
// Linear scaling factor for distance. sqrt((m = 1266730) * 1600)
#define PROX_SQRT_M (UINT32_C(45020))

static uint16_t linearize(uint16_t irradiance) {
  // Results get unstable when c is not reasonably larger than
  // irradiance - simply return max value to indicate "object is far"
  if(irradiance < (PROX_C + 300)) {
    return UINT16_MAX;
  }

  uint32_t irr_take_c = irradiance - PROX_C;

  uint16_t microns =
    (PROX_SQRT_M * sqrt_u32(irr_take_c * 25 * 25)) / irr_take_c;

  return microns;
}

void proximity_init() {
  // Set IR LED current (current = value * 10mA)
  // NOTE: Linearization calculation depends on this value
  vcnl_write(VCNL4000_Reg_LED_Current, 15);

  // Continuous conversion for ambient light
  vcnl_write(VCNL4000_Reg_Ambient_Param, 0x88 | 0x05);
}

/**
 * Take a proximity measurement. Returns distance in micrometres.
 */
uint16_t proximity_measure() {
  vcnl_write(VCNL4000_Reg_Command, _BV(PROX_OD));

  do {
    _delay_ms(1);
  } while(!(vcnl_read(VCNL4000_Reg_Command) & _BV(PROX_DATA_RDY)));

  uint16_t proximity = vcnl_read(VCNL4000_Reg_Proximity_Result_High);
  proximity <<= 8;
  proximity |= vcnl_read(VCNL4000_Reg_Proximity_Result_Low);

  return linearize(proximity);
}

uint16_t proximity_measure_average(uint8_t n_samples) {
#ifdef _DEBUG
  if(n_samples > MAX_PROX_SAMPLES) {
    printf("[PROX] Error - Invalid number of samples\n");
    return 0;
  }
#endif

  uint16_t proximities[MAX_PROX_SAMPLES];

  for(int i = 0; i < n_samples; ++i) {
    proximities[i] = proximity_measure();
  }
  sort(proximities, n_samples);

  return proximities[n_samples / 2];
}

uint16_t als_measure() {
  vcnl_write(VCNL4000_Reg_Command, _BV(ALS_OD));

  do {
    _delay_ms(1);
  } while(!(vcnl_read(VCNL4000_Reg_Command) & _BV(ALS_DATA_RDY)));

  uint16_t als = vcnl_read(VCNL4000_Reg_Ambient_Result_High);
  als <<= 8;
  als |= vcnl_read(VCNL4000_Reg_Ambient_Result_Low);

  return als;
}
