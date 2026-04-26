#include "time_correction.h"
#include "eeprom_storage.h"

TimeCorrection correction;

uint16_t calcDayOfYear(uint8_t day, uint8_t month, uint8_t year) {
	uint16_t dayStamp = day;
	for (uint8_t m = 1; m < month; m++) {
		dayStamp += getMaxMonthDay(m, year);
	}
	return dayStamp;
}

void correction_reset(const DateTime* dateTime) {
	correction.year = dateTime->year;
	correction.stamp = calcDayOfYear(dateTime->day, dateTime->month, correction.year);
	eeprom_update_byte(&eeprom_correctionLastDay, dateTime->day);
	eeprom_update_byte(&eeprom_correctionLastMonth, dateTime->month);
	eeprom_update_byte(&eeprom_correctionLastYear, correction.year);
}

void correction_update(DateTime* dateTime) {
	if (correction.value == 0 || correction.interval == 0) {
		return;
	}
	// Only correct at minutes 3..57 to avoid hour boundary issues
	if (!(dateTime->min > 2 && dateTime->min < 58)) {
		return;
	}

	uint16_t currentDayStamp = calcDayOfYear(dateTime->day, dateTime->month, dateTime->year);
	bool isCanCorrect = false;

	if (dateTime->year == correction.year) {
		if (currentDayStamp >= correction.stamp &&
		    currentDayStamp - correction.stamp >= correction.interval)
		{
			isCanCorrect = true;
		}
	} else {
		// Year boundary crossing
		uint16_t daysInPreviousYear = (correction.year % 4 == 0) ? 366 : 365;
		uint16_t daysSinceCorrection = (daysInPreviousYear - correction.stamp) + currentDayStamp;
		if (daysSinceCorrection >= correction.interval) {
			isCanCorrect = true;
		}
	}

	if (!isCanCorrect) {
		return;
	}

	uint16_t totalSeconds = (uint16_t)dateTime->min * 60 + dateTime->sec;
	if (correction.isForward) {
		totalSeconds += correction.value;
	} else {
		if (totalSeconds < correction.value) return;
		totalSeconds -= correction.value;
	}

	dateTime->sec = totalSeconds % 60;
	dateTime->min = totalSeconds / 60;
	correction_reset(dateTime);
	ds1307_setdate(*dateTime);
}
