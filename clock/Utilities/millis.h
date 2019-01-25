#include <avr/interrupt.h>

uint64_t _millis = 0;
uint16_t _1000us = 0;

void initMillis(void) {
	TCCR0 |= (1<<CS01);
	TIMSK  |= 1<<TOIE0;
	sei();
}

ISR(TIMER0_OVF_vect) {
	_1000us += 256;
	while (_1000us >= 1000) {
		_millis++;
		_1000us -= 1000;
	}
}

uint64_t millis() {
	uint64_t m;
	cli();
	m = _millis;
	sei();
	return m;
}

