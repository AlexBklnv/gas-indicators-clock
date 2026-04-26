#include "night_mode.h"
#include "hardware.h"
#include "millis.h"
#include "adc.h"

NightMode nightMode;

void nightMode_init(void) {
	nightMode.isActive = false;
	nightMode.isCanTryActivate = true;
	nightMode.autoStamp = 0;
}

static void activateNightMode(Brightness* brightness, uint8_t ledBlinkCount) {
	nightMode.isActive = true;
	brightness->level = BRIGHTNESS_MIN;
	if (ledBlinkCount == 0) {
		switchPort(PORTB, PIN_LED, false);
	}
}

void nightMode_update(const DateTime* dateTime, Brightness* brightness,
                      bool isLedActive, uint8_t ledBlinkCount) {
	if (nightMode.modeType == NIGHT_SCHEDULE) {
		if (nightMode.hourStart == nightMode.hourStop) {
			activateNightMode(brightness, ledBlinkCount);
			return;
		} else if (nightMode.hourStart > nightMode.hourStop) {
			if (!(dateTime->hour > nightMode.hourStop &&
			      dateTime->hour <= nightMode.hourStart)) {
				activateNightMode(brightness, ledBlinkCount);
				return;
			}
		} else {
			if (dateTime->hour >= nightMode.hourStart &&
			    dateTime->hour < nightMode.hourStop) {
				activateNightMode(brightness, ledBlinkCount);
				return;
			}
		}
	} else if (nightMode.modeType == NIGHT_AUTO) {
		if (adc_read(ADC_CHANNEL_LIGHT) >= nightMode.threshold) {
			if (nightMode.isCanTryActivate &&
			    millis() - nightMode.autoStamp >= 1000) {
				activateNightMode(brightness, ledBlinkCount);
				return;
			}
		} else {
			nightMode.isCanTryActivate = false;
		}
	}

	if (!nightMode.isCanTryActivate) {
		nightMode.isCanTryActivate = true;
		nightMode.autoStamp = millis();
	}

	if (nightMode.isActive) {
		nightMode.isActive = false;
		brightness->level = brightness->dayLevel;
		if (ledBlinkCount == 0) {
			switchPort(PORTB, PIN_LED, isLedActive);
		}
	}
}

bool nightMode_isActive(void) {
	return nightMode.isActive;
}
