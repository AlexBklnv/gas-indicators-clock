// ============================================================
// menu.h — Project-specific menu navigation and settings
//
// This module is NOT reusable across projects. Each clock
// project has its own set of settings, number of alarms, etc.
// Write a new menu.h/.cpp for each project.
// ============================================================
#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>
#include "buttons.h"
#include "ds1307.h"
#include "display.h"
#include "alarm.h"
#include "night_mode.h"
#include "time_correction.h"

#define MENU_RETURN_TIMEOUT  10

// --- Menu Configuration ---
#define MENU_TRANSITION_ENABLED       1      // Enable displaying group number on transition
#define MENU_TRANSITION_DURATION_MS   1500   // Duration in milliseconds (e.g., 1000-2000)

enum MenuMode : uint8_t {
	MODE_CLOCK = 0,
	MODE_SET_SEC, MODE_SET_MIN, MODE_SET_HOUR,
	MODE_SET_ALARM1_ACTIVE, MODE_SET_ALARM1_MIN, MODE_SET_ALARM1_HOUR,
	MODE_SET_ALARM2_ACTIVE, MODE_SET_ALARM2_MIN, MODE_SET_ALARM2_HOUR,
	MODE_SET_YEAR, MODE_SET_MONTH, MODE_SET_DAY,
	MODE_SET_DATE_SHOW, MODE_SET_DATE_SHOW_STOP, MODE_SET_DATE_SHOW_START,
	MODE_SET_NIGHT_MODE, MODE_SET_NIGHT_STOP, MODE_SET_NIGHT_START,
	MODE_SET_NIGHT_THRESHOLD,
	MODE_SET_CORR_DIR, MODE_SET_CORR_VALUE, MODE_SET_CORR_INTERVAL,
	MODE_LAST
};

struct EditValue {
	bool isGrabbed;
	int16_t value;
	int16_t maxValue;
	int16_t minValue;
};

extern MenuMode menuMode;
extern EditValue editValue;
extern uint8_t menuRank;
extern uint8_t returnTime;
extern uint8_t previousSecond;

#if MENU_TRANSITION_ENABLED
extern bool isMenuTransitioning;
#endif

void menu_init(void);
uint8_t menu_getGroup(MenuMode mode);
void menu_assignEdit(DateTime* dateTime);
void menu_shortPress(ButtonEvent event, DateTime* dateTime,
                     bool nightModeActive, bool* isLedActive);
void menu_longPressButton1(DateTime* dateTime);
void menu_longPressButton2(bool nightModeActive, bool* isLedActive);
bool menu_countdown(uint8_t currentSecond);
void menu_prepareTubeValues(DateTime* dateTime);

#endif // MENU_H
