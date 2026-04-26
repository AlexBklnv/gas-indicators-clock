#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>
#include <stdbool.h>
#include "beeper.h"
#include "ds1307.h"

#ifndef ALARM_COUNT
#define ALARM_COUNT 2
#endif

struct Alarm {
	bool isActive;
	bool isTurnedOff;
	uint8_t lastDay;
	uint8_t startHour;
	uint8_t startMin;
	Beeper beeper;
};

struct HourBeep {
	bool isActive;
	bool manualActivation;
	uint8_t lastHour;
	Beeper beeper;
};

struct LedBlinking {
	bool isCanInit;
	uint8_t count;
	uint8_t startValue;
	uint16_t durationActive;
	uint16_t durationInactive;
	uint32_t ledTime;
};

extern Alarm alarms[ALARM_COUNT];
extern HourBeep hourBeep;
extern LedBlinking ledBlinking;

void alarm_init(void);
void alarm_update(const DateTime* dateTime, bool nightModeActive);
void alarm_dismiss(void);
bool alarm_isAnyBeeping(void);

void hourBeep_init(void);
void hourBeep_update(const DateTime* dateTime, bool nightModeActive);

void ledBlink_init(void);
void ledBlink_update(bool nightModeActive, bool alarmBeeping, bool isLedActive);

#endif // ALARM_H
