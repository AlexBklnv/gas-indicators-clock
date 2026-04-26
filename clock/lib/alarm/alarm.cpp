#include "alarm.h"
#include "hardware.h"
#include "millis.h"

Alarm alarms[ALARM_COUNT];
HourBeep hourBeep;
LedBlinking ledBlinking;

void alarm_init(void) {
	for (uint8_t i = 0; i < ALARM_COUNT; i++) {
		alarms[i].lastDay = 0;
		alarms[i].isTurnedOff = false;
		beeper_init(&alarms[i].beeper, 5, 70, 40, 700);
	}
}

void alarm_update(const DateTime* dateTime, bool nightModeActive) {
	for (uint8_t alarmIndex = 0; alarmIndex < ALARM_COUNT; alarmIndex++) {
		Alarm* currentAlarm = &alarms[alarmIndex];
		if (!currentAlarm->isActive) continue;

		if (dateTime->hour == currentAlarm->startHour &&
		    dateTime->min  == currentAlarm->startMin &&
		    dateTime->day  != currentAlarm->lastDay)
		{
			currentAlarm->lastDay = dateTime->day;
			currentAlarm->beeper.canBeep = true;
			currentAlarm->isTurnedOff = false;
			ledBlinking.isCanInit = false;
			ledBlinking.count = 0;
			currentAlarm->beeper.isCanInit = true;
		} else {
			if (dateTime->min != currentAlarm->startMin) {
				currentAlarm->beeper.canBeep = false;
				currentAlarm->beeper.isCanInit = false;
			}
		}
	}

	// Process beepers: first active alarm takes priority
	for (uint8_t alarmIndex = 0; alarmIndex < ALARM_COUNT; alarmIndex++) {
		Alarm* currentAlarm = &alarms[alarmIndex];
		if (!currentAlarm->isTurnedOff &&
		    currentAlarm->beeper.canBeep &&
		    currentAlarm->isActive)
		{
			ledBlinking.isCanInit = true;
			beeper_stop(&hourBeep.beeper);
			beeper_update(&currentAlarm->beeper);
			return;
		} else if (currentAlarm->isTurnedOff) {
			currentAlarm->beeper.canBeep = false;
		}
	}

	// No alarm beeping — update hour beep
	beeper_update(&hourBeep.beeper);
}

void alarm_dismiss(void) {
	for (uint8_t alarmIndex = 0; alarmIndex < ALARM_COUNT; alarmIndex++) {
		if (alarms[alarmIndex].beeper.canBeep) {
			alarms[alarmIndex].isTurnedOff = true;
			alarms[alarmIndex].beeper.canBeep = false;
		}
	}
}

bool alarm_isAnyBeeping(void) {
	for (uint8_t alarmIndex = 0; alarmIndex < ALARM_COUNT; alarmIndex++) {
		if (alarms[alarmIndex].beeper.canBeep &&
		    alarms[alarmIndex].isActive &&
		    !alarms[alarmIndex].isTurnedOff)
		{
			return true;
		}
	}
	return false;
}

void hourBeep_init(void) {
	hourBeep.manualActivation = false;
	beeper_init(&hourBeep.beeper, 2, 90, 50, 0);
}

void hourBeep_update(const DateTime* dateTime, bool nightModeActive) {
	if (!hourBeep.isActive || nightModeActive) return;

	bool hasAlarmConflict = false;
	for (uint8_t alarmIndex = 0; alarmIndex < ALARM_COUNT; alarmIndex++) {
		if (alarms[alarmIndex].isActive &&
		    alarms[alarmIndex].startMin == 0 &&
		    alarms[alarmIndex].startHour == dateTime->hour)
		{
			hasAlarmConflict = true;
			break;
		}
	}

	if (hourBeep.manualActivation ||
	    (hourBeep.lastHour != dateTime->hour &&
	     dateTime->min == 0 && !hasAlarmConflict))
	{
		hourBeep.manualActivation = false;
		hourBeep.lastHour = dateTime->hour;
		hourBeep.beeper.canBeep = true;
		hourBeep.beeper.isCanInit = true;
	}
}

void ledBlink_init(void) {
	ledBlinking.isCanInit = false;
	ledBlinking.durationActive = 100;
	ledBlinking.durationInactive = 50;
	ledBlinking.count = 0;
	ledBlinking.startValue = 5;
	ledBlinking.ledTime = 0;
}

void ledBlink_update(bool nightModeActive, bool alarmBeeping, bool isLedActive) {
	if (!nightModeActive && !alarmBeeping) {
		switchPort(PORTB, PIN_LED, isLedActive);
		return;
	}

	if (ledBlinking.isCanInit && ledBlinking.count == 0) {
		switchPort(PORTB, PIN_LED, true);
		ledBlinking.ledTime = millis();
		ledBlinking.count = ledBlinking.startValue;
		ledBlinking.isCanInit = false;
	} else {
		if (isPortHigh(PINB, PIN_LED) && ledBlinking.count > 0) {
			if (millis() - ledBlinking.ledTime >= ledBlinking.durationActive) {
				switchPort(PORTB, PIN_LED, false);
				ledBlinking.count--;
				ledBlinking.ledTime = millis();
			}
		} else {
			if (ledBlinking.count > 0) {
				if (millis() - ledBlinking.ledTime >= ledBlinking.durationInactive) {
					switchPort(PORTB, PIN_LED, true);
					ledBlinking.ledTime = millis();
				}
			}
		}
	}
}
