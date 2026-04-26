#ifndef DS1307_H
#define DS1307_H

#include <stdint.h>

#define DS1307_ADDRESS (0x68 << 1)

struct DateTime {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;    // 0-99, represents offset from base century
	uint8_t wday;
};

DateTime ds1307_getdate(void);
void ds1307_setdate(DateTime dateTime);
uint8_t ds1307_decimalToBcd(uint8_t value);
uint8_t ds1307_bcdToDecimal(uint8_t value);

// Returns max days in given month (1-12) for given year (0-99)
uint8_t getMaxMonthDay(uint8_t month, uint8_t year);

#endif // DS1307_H
