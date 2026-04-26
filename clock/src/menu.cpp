// ============================================================
// menu.cpp — Project-specific menu navigation and settings
// ============================================================
#include "menu.h"
#include "hardware.h"
#include "eeprom_storage.h"
#include "adc.h"
#include "millis.h"

MenuMode menuMode;
EditValue editValue;
uint8_t menuRank;
uint8_t returnTime;
uint8_t previousSecond;
#if MENU_TRANSITION_ENABLED
bool isMenuTransitioning = false;
uint32_t menuTransitionStartTime = 0;
#endif

uint8_t menu_getGroup(MenuMode mode) {
	if (mode >= MODE_SET_SEC && mode <= MODE_SET_HOUR) return 1;
	if (mode >= MODE_SET_ALARM1_ACTIVE && mode <= MODE_SET_ALARM1_HOUR) return 2;
	if (mode >= MODE_SET_ALARM2_ACTIVE && mode <= MODE_SET_ALARM2_HOUR) return 3;
	if (mode >= MODE_SET_YEAR && mode <= MODE_SET_DAY) return 4;
	if (mode >= MODE_SET_DATE_SHOW && mode <= MODE_SET_DATE_SHOW_START) return 5;
	if (mode >= MODE_SET_NIGHT_MODE && mode <= MODE_SET_NIGHT_START) return 6;
	if (mode == MODE_SET_NIGHT_THRESHOLD) return 7;
	if (mode >= MODE_SET_CORR_DIR && mode <= MODE_SET_CORR_INTERVAL) return 8;
	return 0;
}

void menu_init(void) {
	menuMode = MODE_CLOCK;
	editValue.isGrabbed = false;
	editValue.value = 0;
	menuRank = 0;
	returnTime = 0;
	previousSecond = 0;
}

static void setEditLimits(int16_t upperLimit, int16_t lowerLimit) {
	editValue.maxValue = upperLimit;
	editValue.minValue = lowerLimit;
}

static void resetAlarmDays(void) {
	for (uint8_t i = 0; i < ALARM_COUNT; i++) alarms[i].lastDay = 0;
}

static void saveDateTimeField(DateTime* dt, uint8_t* field) {
	*field = (uint8_t)editValue.value;
	ds1307_setdate(*dt);
	resetAlarmDays();
	correction_reset(dt);
}

// --- Assign edit values for current menu mode ---

void menu_assignEdit(DateTime* dateTime) {
	switch (menu_getGroup(menuMode)) {
		case 1: tubeMode = TM_SHOW_TIME; break;
		case 2: tubeMode = TM_SHOW_ALARM1; break;
		case 3: tubeMode = TM_SHOW_ALARM2; break;
		case 4: tubeMode = TM_SHOW_DATE; break;
		case 5: tubeMode = TM_SHOW_DATE_SETTINGS; break;
		case 6: tubeMode = TM_SHOW_NIGHT_MODE; break;
		case 7: tubeMode = TM_SHOW_NIGHT_THRESHOLD; break;
		case 8: tubeMode = TM_SHOW_CORRECTION; break;
		default: break;
	}

	// Calculate rank: cycles 3->2->1 per group of 3 modes
	menuRank = 3 - ((menuMode + 2) % 3);
	editValue.isGrabbed = true;

	// Set flash on the edited tube pair
	uint8_t flashStart = menuRank * 2 - 2;
	for (uint8_t i = flashStart; i < flashStart + 2; i++) {
		tube.isFlash[i] = true;
	}

	// --- Assign values based on tubeMode and rank ---

	if (tubeMode == TM_SHOW_TIME) {
		switch (menuRank) {
			case RANK_TUBE_12:
				editValue.value = dateTime->hour;
				setEditLimits(23, 0);
				break;
			case RANK_TUBE_34:
				editValue.value = dateTime->min;
				setEditLimits(59, 0);
				break;
			case RANK_TUBE_56:
				editValue.value = dateTime->sec;
				editValue.isGrabbed = false;
				break;
		}
		return;
	}

	if (tubeMode == TM_SHOW_ALARM1 || tubeMode == TM_SHOW_ALARM2) {
		uint8_t alarmIndex = (tubeMode == TM_SHOW_ALARM1) ? 0 : 1;
		tube.isDoteActive[4] = alarms[alarmIndex].isActive;
		switch (menuRank) {
			case RANK_TUBE_12:
				editValue.value = alarms[alarmIndex].startHour;
				setEditLimits(23, 0);
				break;
			case RANK_TUBE_34:
				editValue.value = alarms[alarmIndex].startMin;
				setEditLimits(59, 0);
				break;
			case RANK_TUBE_56:
				editValue.value = alarms[alarmIndex].isActive;
				setEditLimits(1, 0);
				break;
		}
		return;
	}

	if (tubeMode == TM_SHOW_DATE) {
		uint8_t maxDayValue = getMaxMonthDay(dateTime->month, dateTime->year);
		switch (menuRank) {
			case RANK_TUBE_12:
				editValue.value = dateTime->day > maxDayValue ? maxDayValue : dateTime->day;
				setEditLimits(maxDayValue, 1);  // Day starts from 1
				break;
			case RANK_TUBE_34:
				editValue.value = dateTime->month;
				setEditLimits(12, 1);
				break;
			case RANK_TUBE_56:
				editValue.value = dateTime->year;
				setEditLimits(99, 0);
				break;
		}
		return;
	}

	if (tubeMode == TM_SHOW_DATE_SETTINGS) {
		uint8_t maxStartValue = showDate.stopSecond;
		switch (menuRank) {
			case RANK_TUBE_12:
				editValue.value = showDate.startSecond > maxStartValue ? maxStartValue : showDate.startSecond;
				setEditLimits(maxStartValue, 0);
				break;
			case RANK_TUBE_34:
				editValue.value = showDate.stopSecond;
				setEditLimits(59, 0);
				break;
			case RANK_TUBE_56:
				editValue.value = showDate.isActive;
				setEditLimits(1, 0);
				break;
		}
		return;
	}

	if (tubeMode == TM_SHOW_NIGHT_MODE) {
		switch (menuRank) {
			case RANK_TUBE_12:
				editValue.value = nightMode.hourStart;
				setEditLimits(23, 0);
				break;
			case RANK_TUBE_34:
				editValue.value = nightMode.hourStop;
				setEditLimits(23, 0);
				break;
			case RANK_TUBE_56:
				editValue.value = nightMode.modeType;
				setEditLimits(2, 0);
				break;
		}
		return;
	}

	if (tubeMode == TM_SHOW_NIGHT_THRESHOLD) {
		editValue.value = nightMode.thresholdInit;
		setEditLimits(50, 0);
		return;
	}

	if (tubeMode == TM_SHOW_CORRECTION) {
		switch (menuRank) {
			case RANK_TUBE_12:
				editValue.value = correction.interval;
				setEditLimits(99, 0);
				break;
			case RANK_TUBE_34:
				editValue.value = correction.value;
				setEditLimits(99, 0);
				break;
			case RANK_TUBE_56:
				editValue.value = correction.isForward;
				setEditLimits(1, 0);
				break;
		}
	}
}

// --- Prepare tube display values ---

void menu_prepareTubeValues(DateTime* dateTime) {
#if MENU_TRANSITION_ENABLED
	if (isMenuTransitioning) {
		if (millis() - menuTransitionStartTime < MENU_TRANSITION_DURATION_MS) {
			display_resetFlags();
			tube.isDisabled[0] = true; tube.isDisabled[1] = true;
			tube.isDisabled[4] = true; tube.isDisabled[5] = true;
			uint8_t group = menu_getGroup(menuMode);
			display_fillPair(RANK_TUBE_34, group);
			return;
		} else {
			isMenuTransitioning = false;
			display_resetFlags();
		}
	}
#endif

	uint8_t valueTube12 = 0, valueTube34 = 0, valueTube56 = 0;

	switch (tubeMode) {
		case TM_SHOW_TIME:
			valueTube12 = dateTime->hour;
			valueTube34 = dateTime->min;
			valueTube56 = dateTime->sec;
			break;
		case TM_SHOW_DATE:
			valueTube12 = dateTime->day;
			valueTube34 = dateTime->month;
			valueTube56 = dateTime->year;
			break;
		case TM_SHOW_DATE_SETTINGS:
			valueTube12 = showDate.startSecond;
			valueTube34 = showDate.stopSecond;
			valueTube56 = showDate.isActive;
			break;
		case TM_SHOW_ALARM1:
			valueTube12 = alarms[0].startHour;
			valueTube34 = alarms[0].startMin;
			valueTube56 = alarms[0].isActive;
			tube.isDoteActive[4] = alarms[0].isActive;
			break;
		case TM_SHOW_ALARM2:
			valueTube12 = alarms[1].startHour;
			valueTube34 = alarms[1].startMin;
			valueTube56 = alarms[1].isActive;
			tube.isDoteActive[5] = alarms[1].isActive;
			break;
		case TM_SHOW_CORRECTION:
			valueTube12 = correction.interval;
			valueTube34 = correction.value;
			valueTube56 = correction.isForward;
			break;
		case TM_SHOW_NIGHT_MODE:
			valueTube12 = nightMode.hourStart;
			valueTube34 = nightMode.hourStop;
			valueTube56 = nightMode.modeType;
			break;
		case TM_ETCHING:
			valueTube12 = etching.value;
			valueTube34 = etching.value;
			valueTube56 = etching.value;
			break;
		case TM_SHOW_NIGHT_THRESHOLD:
			valueTube12 = 0;
			valueTube34 = 0;
			valueTube56 = nightMode.thresholdInit;
			{
				int16_t testThreshold = 20 * editValue.value;
				switchPort(PORTB, PIN_LED, (adc_read(ADC_CHANNEL_LIGHT) < testThreshold));
			}
			break;
	}

	display_fillPair(RANK_TUBE_12, valueTube12);
	display_fillPair(RANK_TUBE_34, valueTube34);
	display_fillPair(RANK_TUBE_56, valueTube56);
}

// --- Short press handling ---

void menu_shortPress(ButtonEvent event, DateTime* dateTime,
                     bool nightModeActive, bool* isLedActive) {
	if (menuMode == MODE_CLOCK) {
		switch (event) {
			case BTN_SHORT_1:
				hourBeep.isActive = !hourBeep.isActive;
				eeprom_update_byte(&eeprom_isHourBeepActive, hourBeep.isActive);
				if (nightModeActive) {
					ledBlinking.isCanInit = true;
					hourBeep.manualActivation = false;
				} else {
					hourBeep.manualActivation = hourBeep.isActive;
				}
				break;
			case BTN_SHORT_2:
				if (!nightModeActive) {
					brightness.level = brightness.level >= BRIGHTNESS_MAX
					                   ? BRIGHTNESS_MIN
					                   : brightness.level + 1;
					brightness.dayLevel = brightness.level;
					eeprom_update_byte(&eeprom_bright, brightness.level);
				} else {
					ledBlinking.isCanInit = true;
				}
				break;
		}
		return;
	}

	if (menuMode == MODE_SET_SEC) {
		// Reset seconds, round up minute if sec >= 30
		if (dateTime->sec >= 30) {
			resetAlarmDays();
			if (dateTime->min == 59) {
				dateTime->min = 0;
				if (dateTime->hour == 23) {
					dateTime->hour = 0;
					if (dateTime->day == getMaxMonthDay(dateTime->month, dateTime->year)) {
						dateTime->day = 1;
						if (dateTime->month == 12) {
							dateTime->month = 1;
							dateTime->year = (dateTime->year == 99) ? 0 : dateTime->year + 1;
						} else {
							dateTime->month++;
						}
					} else {
						dateTime->day++;
					}
				} else {
					dateTime->hour++;
				}
				hourBeep.lastHour = dateTime->hour;
			} else {
				dateTime->min++;
			}
			etching.lastMinute = dateTime->min;
		}
		dateTime->sec = 0;
		editValue.value = 0;
		previousSecond = 0;
		ds1307_setdate(*dateTime);
		correction_reset(dateTime);
		return;
	}

	// Value increment/decrement for other modes
	switch (event) {
		case BTN_SHORT_1: editValue.value++; break;
		case BTN_SHORT_2: editValue.value--; break;
		default: break;
	}

	if (editValue.value > editValue.maxValue) {
		editValue.value = editValue.minValue;
	} else if (editValue.value < editValue.minValue) {
		editValue.value = editValue.maxValue;
	}
}

// --- Long press button 1: confirm & advance ---

void menu_longPressButton1(DateTime* dateTime) {
	switch (menuMode) {
		case MODE_SET_MIN:
			saveDateTimeField(dateTime, &dateTime->min);
			etching.lastMinute = dateTime->min;
			hourBeep.lastHour = dateTime->hour;
			break;
		case MODE_SET_HOUR:
			saveDateTimeField(dateTime, &dateTime->hour);
			hourBeep.lastHour = dateTime->hour;
			break;
		case MODE_SET_YEAR:
			saveDateTimeField(dateTime, &dateTime->year);
			break;
		case MODE_SET_MONTH:
			saveDateTimeField(dateTime, &dateTime->month);
			break;
		case MODE_SET_DAY:
			saveDateTimeField(dateTime, &dateTime->day);
			break;
		case MODE_SET_ALARM1_ACTIVE:
			alarms[0].isActive = editValue.value;
			alarms[0].lastDay = 0;
			eeprom_update_byte(&eeprom_isAlarm1Active, alarms[0].isActive);
			break;
		case MODE_SET_ALARM1_MIN:
			alarms[0].startMin = editValue.value;
			alarms[0].lastDay = 0;
			eeprom_update_byte(&eeprom_alarm1Min, alarms[0].startMin);
			break;
		case MODE_SET_ALARM1_HOUR:
			alarms[0].startHour = editValue.value;
			alarms[0].lastDay = 0;
			eeprom_update_byte(&eeprom_alarm1Hour, alarms[0].startHour);
			break;
		case MODE_SET_ALARM2_ACTIVE:
			alarms[1].isActive = editValue.value;
			alarms[1].lastDay = 0;
			eeprom_update_byte(&eeprom_isAlarm2Active, alarms[1].isActive);
			break;
		case MODE_SET_ALARM2_MIN:
			alarms[1].startMin = editValue.value;
			alarms[1].lastDay = 0;
			eeprom_update_byte(&eeprom_alarm2Min, alarms[1].startMin);
			break;
		case MODE_SET_ALARM2_HOUR:
			alarms[1].startHour = editValue.value;
			alarms[1].lastDay = 0;
			eeprom_update_byte(&eeprom_alarm2Hour, alarms[1].startHour);
			break;
		case MODE_SET_DATE_SHOW:
			showDate.isActive = editValue.value;
			eeprom_update_byte(&eeprom_showDateIsActive, showDate.isActive);
			break;
		case MODE_SET_DATE_SHOW_STOP:
			showDate.stopSecond = editValue.value;
			eeprom_update_byte(&eeprom_showDateStop, showDate.stopSecond);
			break;
		case MODE_SET_DATE_SHOW_START:
			showDate.startSecond = editValue.value;
			eeprom_update_byte(&eeprom_showDateStart, showDate.startSecond);
			break;
		case MODE_SET_CORR_DIR:
			correction.isForward = editValue.value;
			eeprom_update_byte(&eeprom_correctionIsForward, correction.isForward);
			correction_reset(dateTime);
			break;
		case MODE_SET_CORR_VALUE:
			correction.value = editValue.value;
			eeprom_update_byte(&eeprom_correctionValue, correction.value);
			correction_reset(dateTime);
			break;
		case MODE_SET_CORR_INTERVAL:
			correction.interval = editValue.value;
			eeprom_update_byte(&eeprom_correctionInterval, correction.interval);
			correction_reset(dateTime);
			break;
		case MODE_SET_NIGHT_MODE:
			nightMode.modeType = (NightModeType)editValue.value;
			eeprom_update_byte(&eeprom_nightMode, nightMode.modeType);
			break;
		case MODE_SET_NIGHT_STOP:
			nightMode.hourStop = editValue.value;
			eeprom_update_byte(&eeprom_hourNightModeStop, nightMode.hourStop);
			break;
		case MODE_SET_NIGHT_START:
			nightMode.hourStart = editValue.value;
			eeprom_update_byte(&eeprom_hourNightModeStart, nightMode.hourStart);
			break;
		case MODE_SET_NIGHT_THRESHOLD:
			nightMode.thresholdInit = editValue.value;
			nightMode.threshold = 20 * nightMode.thresholdInit;
			eeprom_update_byte(&eeprom_nightThreshold, nightMode.thresholdInit);
			break;
		default:
			break;
	}

	// Advance to next mode
#if MENU_TRANSITION_ENABLED
	uint8_t oldGroup = menu_getGroup(menuMode);
#endif
	
	if (menuMode == MODE_SET_NIGHT_THRESHOLD) {
		menuMode = MODE_SET_CORR_DIR;
	} else {
		uint8_t nextMode = (uint8_t)menuMode + 1;
		menuMode = (nextMode < (uint8_t)MODE_LAST) ? (MenuMode)nextMode : MODE_CLOCK;
	}
	
#if MENU_TRANSITION_ENABLED
	uint8_t newGroup = menu_getGroup(menuMode);
	if (newGroup != 0 && newGroup != oldGroup) {
		isMenuTransitioning = true;
		menuTransitionStartTime = millis();
	}
#endif

	display_resetFlags();
	editValue.isGrabbed = false;
}

void menu_longPressButton2(bool nightModeActive, bool* isLedActive) {
	if (menuMode == MODE_CLOCK && !nightModeActive) {
		*isLedActive = !(*isLedActive);
		switchPort(PORTB, PIN_LED, *isLedActive);
		eeprom_update_byte(&eeprom_ledState, *isLedActive);
	}
}

bool menu_countdown(uint8_t currentSecond) {
	if (menuMode == MODE_CLOCK) return false;

	if (previousSecond != currentSecond) {
		previousSecond = currentSecond;
		if (returnTime == 0) {
			menuMode = MODE_CLOCK;
			display_resetFlags();
			return true;
		} else {
			returnTime--;
		}
	}
	return false;
}
