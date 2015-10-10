#ifndef __PROXIMITY_H__
#define __PROXIMITY_H__

#include <stdint.h>

void proximity_init();
uint16_t proximity_measure();
uint16_t proximity_measure_average(uint8_t n_samples);

uint16_t als_measure();

#endif
