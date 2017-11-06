#include <avr/io.h>

#define TRUE 1
#define FALSE 0

#include "rtc.h"

#define RTC_ADDR 0x68 // I2C address
#define CH_BIT 7 // clock halt bit


uint8_t dec2bcd(uint8_t d)
{
  return ((d/10 * 16) + (d % 10));
}

uint8_t bcd2dec(uint8_t b)
{
  return ((b/16 * 10) + (b % 16));
}

uint8_t rtc_read_byte(uint8_t offset)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(offset);
	twi_end_transmission();
	
	twi_request_from(RTC_ADDR, 1);
	return twi_receive();
}

void rtc_write_byte(uint8_t b, uint8_t offset)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(offset);
	twi_send_byte(b);
	twi_end_transmission();
}


void rtc_init(void) {
	
	uint8_t temp1 = rtc_read_byte(0x11);
	uint8_t temp2 = rtc_read_byte(0x12);
	
	rtc_write_byte(0xee, 0x11);
	rtc_write_byte(0xdd, 0x12);

	if (rtc_read_byte(0x11) == 0xee && rtc_read_byte(0x12) == 0xdd) {
		// restore values
		rtc_write_byte(temp1, 0x11);
		rtc_write_byte(temp2, 0x12);
	}
}

// fixme: support 12-hour mode for setting time
void rtc_set_time(DateTime tm_)
{
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0);
	
	// clock halt bit is 7th bit of seconds: this is always cleared to start the clock
	twi_send_byte(dec2bcd(tm_.sec)); // seconds
	twi_send_byte(dec2bcd(tm_.min)); // minutes
	twi_send_byte(dec2bcd(tm_.hour)); // hours
	twi_send_byte(dec2bcd(tm_.wday)); // day of week
	twi_send_byte(dec2bcd(tm_.day)); // day
	twi_send_byte(dec2bcd(tm_.month)); // month
	twi_send_byte(dec2bcd(tm_.year)); // year

	twi_end_transmission();
}

DateTime rtc_get_time()
{
	uint8_t rtc[9];
	DateTime dateTime;
	// read 7 bytes starting from register 0
	// sec, min, hour, day-of-week, date, month, year
	twi_begin_transmission(RTC_ADDR);
	twi_send_byte(0x0);
	twi_end_transmission();

	twi_request_from(RTC_ADDR, 7);

	for (uint8_t i = 0; i < 7; i++) {
		rtc[i] = twi_receive();
	}

	twi_end_transmission();

	// Clear clock halt bit from read data
	// This starts the clock for a DS1307, and has no effect for a DS3231
	rtc[0] &= ~(_BV(CH_BIT)); // clear bit

	dateTime.sec = bcd2dec(rtc[0]);
	dateTime.min = bcd2dec(rtc[1]);
	dateTime.hour = bcd2dec(rtc[2]);
	dateTime.day = bcd2dec(rtc[4]);
	dateTime.month = bcd2dec(rtc[5] & 0x1F); // returns 1-12
	dateTime.year = bcd2dec(rtc[6]);//century == 1 ? 2000 + bcd2dec(rtc[6]) : 1900 + bcd2dec(rtc[6]); // year 0-99
	dateTime.wday = bcd2dec(rtc[3]); // returns 1-7

	return dateTime;
}



// DS1307 only (has no effect when run on DS3231)
// halt/start the clock
// 7th bit of register 0 (second register)
// 0 = clock is running
// 1 = clock is not running
void rtc_run_clock(bool run)
{
   uint8_t b = rtc_read_byte(0x0);

  if (run)
    b &= ~(_BV(CH_BIT)); // clear bit
  else
    b |= _BV(CH_BIT); // set bit
    
    rtc_write_byte(b, 0x0);
}

// DS1307 only
// Returns true if the clock is running, false otherwise
// For DS3231, it always returns true
bool rtc_is_clock_running(void)
{
 
  uint8_t b = rtc_read_byte(0x0);

  if (b & _BV(CH_BIT)) return false;
  return true;
}



