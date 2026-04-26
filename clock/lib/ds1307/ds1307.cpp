#include "ds1307.h"
#include "i2c.h"

uint8_t ds1307_decimalToBcd(uint8_t value) {
	return value + 6 * (value / 10);
}

uint8_t ds1307_bcdToDecimal(uint8_t value) {
	return value - 6 * (value >> 4);
}

void ds1307_setdate(DateTime dateTime) {
	i2c_start_wait(DS1307_ADDRESS | I2C_WRITE);
	i2c_write(0x00);
	i2c_write(ds1307_decimalToBcd(dateTime.sec));
	i2c_write(ds1307_decimalToBcd(dateTime.min));
	i2c_write(ds1307_decimalToBcd(dateTime.hour));
	i2c_write(ds1307_decimalToBcd(1));        // day of week (unused)
	i2c_write(ds1307_decimalToBcd(dateTime.day));
	i2c_write(ds1307_decimalToBcd(dateTime.month));
	i2c_write(ds1307_decimalToBcd(dateTime.year));
	i2c_write(0x00);                          // control register
	i2c_stop();
}

DateTime ds1307_getdate(void) {
	i2c_start_wait(DS1307_ADDRESS | I2C_WRITE);
	i2c_write(0x00);
	i2c_stop();

	DateTime dateTime;
	i2c_rep_start(DS1307_ADDRESS | I2C_READ);
	dateTime.sec   = ds1307_bcdToDecimal(i2c_readAck() & 0x7F);
	dateTime.min   = ds1307_bcdToDecimal(i2c_readAck());
	dateTime.hour  = ds1307_bcdToDecimal(i2c_readAck());
	i2c_readAck(); // skip day of week
	dateTime.day   = ds1307_bcdToDecimal(i2c_readAck());
	dateTime.month = ds1307_bcdToDecimal(i2c_readAck());
	dateTime.year  = ds1307_bcdToDecimal(i2c_readNak());
	i2c_stop();
	return dateTime;
}

uint8_t getMaxMonthDay(uint8_t month, uint8_t year) {
	if (month == 2) {
		return (year % 4 == 0) ? 29 : 28;
	}
	if (month < 8) {
		return (month % 2 == 0) ? 30 : 31;
	}
	return (month % 2 == 1) ? 30 : 31;
}
