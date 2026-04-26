#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);
int16_t adc_read(uint8_t channel);

#endif // ADC_H
