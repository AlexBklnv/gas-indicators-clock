#include "display.h"
#include "hardware.h"
#include "millis.h"
#include <string.h>

Brightness brightness;
Tube tube;
Etching etching;
ShowDate showDate;
TubeMode tubeMode;

void display_init(void) {
	display_resetFlags();
	memset(tube.value, 0, sizeof(tube.value));
	tube.switchTime = 0;

	etching.isWorking = false;
	etching.duration = ETCHING_DURATION_MS;
	etching.lastMinute = 0;
	etching.value = 0;
	etching.switchTime = 0;

	tubeMode = TM_SHOW_TIME;
}

void display_resetFlags(void) {
	memset(tube.isFlash, 0, sizeof(tube.isFlash));
	memset(tube.isDisabled, 0, sizeof(tube.isDisabled));
	memset(tube.isDoteActive, 0, sizeof(tube.isDoteActive));
}

void display_fillPair(uint8_t pairIndex, uint8_t value) {
	if (pairIndex < 1 || pairIndex > TUBE_PAIR_COUNT) return;
	uint8_t startIndex = pairIndex * 2 - 2;
	tube.value[startIndex]     = value < 10 ? 0 : value / 10;
	tube.value[startIndex + 1] = value < 10 ? value : value % 10;
}

static void decodeToBinary(uint8_t digit) {
	tube.decoderBits[3] = digit & 0x01;
	tube.decoderBits[2] = (digit >> 1) & 0x01;
	tube.decoderBits[1] = (digit >> 2) & 0x01;
	tube.decoderBits[0] = (digit >> 3) & 0x01;
}

static void tubeSwitch(uint8_t menuMode) {
	// Brightness delay — longer delay = brighter
	switch (brightness.level) {
		case 0: _delay_us(1);   break;
		case 1: _delay_us(266); break;
		case 2: _delay_us(533); break;
		case 3: _delay_us(800); break;
	}

	// Turn off all anodes
	switchPort(PORTC, PIN_ANODE_PAIR_14, 1);
	switchPort(PORTC, PIN_ANODE_PAIR_25, 1);
	switchPort(PORTC, PIN_ANODE_PAIR_36, 1);

	// Flashing logic for edit mode
	if (menuMode > 0) {
		uint32_t now = millis();
		for (uint8_t i = 0; i < TUBE_COUNT; i++) {
			if (tube.isFlash[i]) {
				if (now - tube.switchTime >= 150) {
					tube.isDisabled[i] = true;
					if (now - tube.switchTime >= 550) {
						tube.switchTime = now;
					}
				} else {
					tube.isDisabled[i] = false;
				}
			} else {
				tube.isDisabled[i] = false;
			}
		}
	}

	_delay_us(800);

	// Reset all decoder and dot pins
	switchPort(PORTB, PIN_DECODER_LOW_0, 1);
	switchPort(PORTD, PIN_DECODER_LOW_1, 1);
	switchPort(PORTD, PIN_DECODER_LOW_2, 1);
	switchPort(PORTB, PIN_DECODER_LOW_3, 1);
	switchPort(PORTD, PIN_DECODER_HIGH_0, 1);
	switchPort(PORTD, PIN_DECODER_HIGH_1, 1);
	switchPort(PORTB, PIN_DECODER_HIGH_2, 1);
	switchPort(PORTB, PIN_DECODER_HIGH_3, 1);
	switchPort(PORTC, PIN_DOT_LOW, 0);
	switchPort(PORTD, PIN_DOT_HIGH, 0);

	_delay_us(400);
}

void display_render(uint8_t menuMode, uint8_t rank, int16_t editValue) {
	// Override edited pair with edit value
	if (menuMode != 0) {
		display_fillPair(rank, (uint8_t)editValue);
	}

	// Render 3 tube pairs
	for (uint8_t pairIndex = 0; pairIndex < TUBE_PAIR_COUNT; pairIndex++) {
		// High bank tube (tubes 1, 2, 3)
		if (!tube.isDisabled[pairIndex]) {
			decodeToBinary(tube.value[pairIndex]);
			switchPort(PORTD, PIN_DOT_HIGH, tube.isDoteActive[pairIndex]);
			switchPort(PORTD, PIN_DECODER_HIGH_0, tube.decoderBits[3]);
			switchPort(PORTD, PIN_DECODER_HIGH_1, tube.decoderBits[2]);
			switchPort(PORTB, PIN_DECODER_HIGH_2, tube.decoderBits[1]);
			switchPort(PORTB, PIN_DECODER_HIGH_3, tube.decoderBits[0]);
		}
		// Low bank tube (tubes 4, 5, 6)
		if (!tube.isDisabled[pairIndex + 3]) {
			decodeToBinary(tube.value[pairIndex + 3]);
			switchPort(PORTC, PIN_DOT_LOW, tube.isDoteActive[pairIndex + 3]);
			switchPort(PORTB, PIN_DECODER_LOW_0, tube.decoderBits[3]);
			switchPort(PORTD, PIN_DECODER_LOW_1, tube.decoderBits[2]);
			switchPort(PORTD, PIN_DECODER_LOW_2, tube.decoderBits[1]);
			switchPort(PORTB, PIN_DECODER_LOW_3, tube.decoderBits[0]);
		}
		// Enable anode for this pair
		switch (pairIndex) {
			case 0: switchPort(PORTC, PIN_ANODE_PAIR_14, 0); break;
			case 1: switchPort(PORTC, PIN_ANODE_PAIR_25, 0); break;
			case 2: switchPort(PORTC, PIN_ANODE_PAIR_36, 0); break;
		}
		tubeSwitch(menuMode);
	}
}

// --- Etching (anti-cathode-poisoning) ---

void etching_check(uint8_t currentMinute, uint8_t currentSecond) {
	if (currentMinute % ETCHING_INTERVAL_MIN == 0 &&
	    currentSecond < 3 &&
	    etching.lastMinute != currentMinute)
	{
		etching.lastMinute = currentMinute;
		etching.isWorking = true;
		etching.switchTime = millis();
		etching.value = 0;
	}
}

bool etching_update(void) {
	if (!etching.isWorking) return false;

	if (etching.value > 99) {
		etching.isWorking = false;
		return false;
	}

	if (millis() - etching.switchTime >= etching.duration) {
		etching.value += 11;
		etching.switchTime = millis();
	}
	return true;
}
