#include "millis.h"
#include <avr/interrupt.h>
#include <avr/io.h>

static volatile uint32_t millisCounter = 0;
static volatile uint16_t microsAccumulator = 0;

void initMillis(void) {
	TCCR0 |= (1 << CS01);    // prescaler /8
	TIMSK |= (1 << TOIE0);   // enable Timer0 overflow interrupt
	sei();
}

ISR(TIMER0_OVF_vect) {
	microsAccumulator += 256;
	while (microsAccumulator >= 1000) {
		millisCounter++;
		microsAccumulator -= 1000;
	}
}

uint32_t millis(void) {
	uint32_t result;
	cli();
	result = millisCounter;
	sei();
	return result;
}
