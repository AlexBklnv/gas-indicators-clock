#ifndef BEEPER_H
#define BEEPER_H

#include <stdint.h>
#include <stdbool.h>

struct Beeper {
	bool isCanInit;
	bool canBeep;
	bool isPinActive;
	uint8_t count;
	uint8_t initCount;
	uint16_t durationActive;
	uint16_t durationInactive;
	uint16_t waitingTime;
	uint32_t beepTime;
};

void beeper_init(Beeper* beeper, uint8_t initCount, uint16_t durationActive,
                 uint16_t durationInactive, uint16_t waitingTime);
void beeper_start(Beeper* beeper);
void beeper_stop(Beeper* beeper);
void beeper_update(Beeper* beeper);
bool beeper_isActive(const Beeper* beeper);

#endif // BEEPER_H
