#include "adc.h"
#include <avr/io.h>

void adc_init(void) {
	ADMUX = (1 << REFS0);  // AVCC as reference
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

int16_t adc_read(uint8_t channel) {
	ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
	ADCSRA |= (1 << ADSC);
	uint16_t timeout = 1000;
	while ((ADCSRA & (1 << ADSC)) && timeout--);
	return ADCW;
}
