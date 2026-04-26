#ifndef TIME_CORRECTION_H
#define TIME_CORRECTION_H

#include <stdint.h>
#include <stdbool.h>
#include "ds1307.h"

struct TimeCorrection {
	bool isForward;
	uint8_t value;
	uint8_t interval;
	uint8_t year;
	uint16_t stamp;
};

extern TimeCorrection correction;

void correction_update(DateTime* dateTime);
void correction_reset(const DateTime* dateTime);

// Calculate day-of-year (1-366) for given date
uint16_t calcDayOfYear(uint8_t day, uint8_t month, uint8_t year);

#endif // TIME_CORRECTION_H
