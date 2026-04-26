#include "hardware.h"
#include "millis.h"
#include "i2c.h"
#include "ds1307.h"
#include "adc.h"
#include "eeprom_storage.h"
#include "display.h"
#include "buttons.h"
#include "beeper.h"
#include "alarm.h"
#include "night_mode.h"
#include "time_correction.h"
#include "menu.h"

// --- Global state ---
static DateTime dateTime;
static bool isLedActive;

// --- Initialization ---

static void initHardwarePorts(void) {
	DDRD = INIT_DDRD;
	DDRB = INIT_DDRB;
	DDRC = INIT_DDRC;
}

static void loadSettingsFromEeprom(void) {
	// Brightness
	brightness.level = eeprom_readSafe(&eeprom_bright, 3, BRIGHTNESS_MAX);
	brightness.dayLevel = brightness.level;

	// LED
	isLedActive = eeprom_readSafe(&eeprom_ledState, 1, 1);

	// Hour beep
	hourBeep.isActive = eeprom_readSafe(&eeprom_isHourBeepActive, 1, 1);
	hourBeep.lastHour = dateTime.hour;

	// Alarm 1
	alarms[0].isActive  = eeprom_readSafe(&eeprom_isAlarm1Active, 0, 1);
	alarms[0].startHour = eeprom_readSafe(&eeprom_alarm1Hour, 7, 23);
	alarms[0].startMin  = eeprom_readSafe(&eeprom_alarm1Min, 0, 59);

	// Alarm 2
	alarms[1].isActive  = eeprom_readSafe(&eeprom_isAlarm2Active, 0, 1);
	alarms[1].startHour = eeprom_readSafe(&eeprom_alarm2Hour, 8, 23);
	alarms[1].startMin  = eeprom_readSafe(&eeprom_alarm2Min, 0, 59);

	// Time correction
	correction.isForward = eeprom_readSafe(&eeprom_correctionIsForward, 1, 1);
	correction.value     = eeprom_read_byte(&eeprom_correctionValue);
	correction.interval  = eeprom_read_byte(&eeprom_correctionInterval);
	correction.year      = eeprom_read_byte(&eeprom_correctionLastYear);
	correction.stamp     = calcDayOfYear(
		eeprom_read_byte(&eeprom_correctionLastDay),
		eeprom_read_byte(&eeprom_correctionLastMonth),
		correction.year
	);

	// Night mode
	nightMode.hourStart     = eeprom_readSafe(&eeprom_hourNightModeStart, 22, 23);
	nightMode.hourStop      = eeprom_readSafe(&eeprom_hourNightModeStop, 7, 23);
	nightMode.modeType      = (NightModeType)eeprom_readSafe(&eeprom_nightMode, 2, 2);
	nightMode.thresholdInit = eeprom_readSafe(&eeprom_nightThreshold, 25, 50);
	nightMode.threshold     = 20 * nightMode.thresholdInit;

	// Show date
	showDate.isActive     = eeprom_readSafe(&eeprom_showDateIsActive, 1, 1);
	showDate.startSecond  = eeprom_readSafe(&eeprom_showDateStart, 51, 59);
	showDate.stopSecond   = eeprom_readSafe(&eeprom_showDateStop, 53, 59);
}

static void init(void) {
	initHardwarePorts();
	adc_init();
	i2c_init();
	_delay_us(100);

	dateTime = ds1307_getdate();

	// Initialize all modules
	display_init();
	buttons_init();
	alarm_init();
	hourBeep_init();
	ledBlink_init();
	nightMode_init();
	menu_init();

	// Load saved settings
	loadSettingsFromEeprom();

	// Apply initial state
	switchPort(PORTB, PIN_LED, isLedActive);
	etching.lastMinute = dateTime.min;

	correction_reset(&dateTime);
	initMillis();
}

// --- Main loop ---

int main(void) {
	init();

	while (true) {
		dateTime = ds1307_getdate();

		// Read buttons (blocked during etching animation)
		ButtonEvent buttonEvent = buttons_update(etching.isWorking);

		// Dismiss alarm on any button press
		if (buttonEvent == BTN_DEBOUNCED && alarm_isAnyBeeping()) {
			alarm_dismiss();
			buttons_consume();
			buttonEvent = BTN_NONE;
		}

		if (menuMode == MODE_CLOCK) {
			// --- Normal clock operation ---
			nightMode_update(&dateTime, &brightness, isLedActive, ledBlinking.count);
			correction_update(&dateTime);
			ledBlink_update(nightMode_isActive(), alarm_isAnyBeeping(), isLedActive);

			alarm_update(&dateTime, nightMode_isActive());
			hourBeep_update(&dateTime, nightMode_isActive());

			// Dot indicators show alarm status
			tube.isDoteActive[4] = alarms[0].isActive;
			tube.isDoteActive[5] = alarms[1].isActive || hourBeep.isActive;

			// Default display: time
			tubeMode = TM_SHOW_TIME;

			// Periodic date display
			if (showDate.isActive) {
				if (dateTime.sec >= showDate.startSecond &&
				    dateTime.sec <= showDate.stopSecond) {
					tubeMode = TM_SHOW_DATE;
				}
			}

			// Anti-cathode-poisoning
			etching_check(dateTime.min, dateTime.sec);
			if (etching_update()) {
				tubeMode = TM_ETCHING;
			}
		} else {
			// --- Settings mode ---
			menu_countdown(dateTime.sec);

			if (!editValue.isGrabbed) {
				menu_assignEdit(&dateTime);
			}
		}

		// Handle button events
		if (buttonEvent == BTN_SHORT_1 || buttonEvent == BTN_SHORT_2) {
			returnTime = MENU_RETURN_TIMEOUT;
			menu_shortPress(buttonEvent, &dateTime, nightMode_isActive(), &isLedActive);
		}
		if (buttonEvent == BTN_LONG_1) {
			returnTime = MENU_RETURN_TIMEOUT;
			menu_longPressButton1(&dateTime);
		}
		if (buttonEvent == BTN_LONG_2) {
			returnTime = MENU_RETURN_TIMEOUT;
			menu_longPressButton2(nightMode_isActive(), &isLedActive);
		}

		// Render display
		menu_prepareTubeValues(&dateTime);
#if MENU_TRANSITION_ENABLED
		if (isMenuTransitioning) {
			display_render(0, menuRank, 0);
		} else {
			display_render((uint8_t)menuMode, menuRank, editValue.value);
		}
#else
		display_render((uint8_t)menuMode, menuRank, editValue.value);
#endif
	}
}
