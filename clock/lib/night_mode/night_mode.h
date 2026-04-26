#ifndef NIGHT_MODE_H
#define NIGHT_MODE_H

#include <stdint.h>
#include <stdbool.h>
#include "display.h"
#include "ds1307.h"

enum NightModeType : uint8_t {
	NIGHT_OFF = 0,
	NIGHT_SCHEDULE = 1,
	NIGHT_AUTO = 2
};

struct NightMode {
	bool isActive;
	bool isCanTryActivate;
	NightModeType modeType;
	int16_t threshold;
	uint8_t thresholdInit;
	uint8_t hourStart;
	uint8_t hourStop;
	uint32_t autoStamp;
};

extern NightMode nightMode;

void nightMode_init(void);
void nightMode_update(const DateTime* dateTime, Brightness* brightness,
                      bool isLedActive, uint8_t ledBlinkCount);
bool nightMode_isActive(void);

#endif // NIGHT_MODE_H
