#include <avr/interrupt.h>

uint64_t _millis = 0;
uint16_t _1000us = 0;

void initMillis(void) {
	/* interrup setup */
	// prescale timer0 to 1/8th the clock rate
	// overflow timer0 every 0.256 ms
	TCCR0 |= (1<<CS01);
	// enable timer overflow interrupt
	TIMSK  |= 1<<TOIE0;
	// Enable global interrupts
	sei();
}

/* interrupts routines */
// timer overflow occur every 0.256 ms
ISR(TIMER0_OVF_vect) {
	_1000us += 256;
	while (_1000us > 1000) {
		_millis++;
		_1000us -= 1000;
	}
}

// safe access to millis counter
uint64_t millis() {
	uint64_t m;
	cli();
	m = _millis;
	sei();
	return m;
}

