/*
 * DS RTC Library: DS1307 and DS3231 driver library
 * (C) 2011 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

#ifndef DS1307_H
#define DS1307_H

#include <stdbool.h>
#include <avr/io.h>
#include "../TWI/twi.h"

#define DS1307_SLAVE_ADDR 0b11010000

struct DateTime {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint8_t wday;
};

// Initialize the RTC and autodetect type (DS1307 or DS3231)
void rtc_init(void);

DateTime rtc_get_time();
void rtc_set_time(DateTime);


// start/stop clock running (DS1307 only)
void rtc_run_clock(bool run);
bool rtc_is_clock_running(void);

#endif
