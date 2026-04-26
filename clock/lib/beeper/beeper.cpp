#include "beeper.h"
#include "hardware.h"
#include "millis.h"

void beeper_init(Beeper* beeper, uint8_t initCount, uint16_t durationActive,
                 uint16_t durationInactive, uint16_t waitingTime) {
	beeper->initCount = initCount;
	beeper->durationActive = durationActive;
	beeper->durationInactive = durationInactive;
	beeper->waitingTime = waitingTime;
	beeper->canBeep = false;
	beeper->isCanInit = false;
	beeper->isPinActive = false;
	beeper->count = 0;
	beeper->beepTime = 0;
}

void beeper_start(Beeper* beeper) {
	beeper->canBeep = true;
	beeper->isCanInit = true;
}

void beeper_stop(Beeper* beeper) {
	beeper->canBeep = false;
	beeper->isCanInit = false;
}

bool beeper_isActive(const Beeper* beeper) {
	return beeper->canBeep;
}

void beeper_update(Beeper* beeper) {
	if (!beeper->canBeep) {
		switchPort(PORTD, PIN_BUZZER, 0);
		beeper->isPinActive = false;
		return;
	}

	if (beeper->isCanInit) {
		switchPort(PORTD, PIN_BUZZER, 1);
		beeper->isPinActive = true;
		beeper->beepTime = millis();
		beeper->count = beeper->initCount - 1;
		beeper->isCanInit = false;
	} else {
		if (beeper->isPinActive) {
			if (millis() - beeper->beepTime >= beeper->durationActive) {
				switchPort(PORTD, PIN_BUZZER, 0);
				beeper->isPinActive = false;
				beeper->beepTime = millis();
			}
		} else {
			if (beeper->count > 0) {
				if (millis() - beeper->beepTime >= beeper->durationInactive) {
					switchPort(PORTD, PIN_BUZZER, 1);
					beeper->isPinActive = true;
					beeper->beepTime = millis();
					beeper->count--;
				}
			} else if (beeper->waitingTime != 0 &&
			           millis() - beeper->beepTime >= beeper->waitingTime) {
				beeper->isCanInit = true;
			}
		}
	}
}
