#ifndef __ADC_H__
#define __ADC_H__

#include <stdint.h>

void adc_init();
uint16_t adc_read(uint8_t channel);

#endif
