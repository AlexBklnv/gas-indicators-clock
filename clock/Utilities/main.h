#define F_CPU 8000000UL
#include "i2c.h"
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "millis.h"

#define DS1307_ADDR (0x68<<1) 
#define switchPort(port,pd, onOff) if (onOff == 1) port|= _BV(pd); else port&=~_BV(pd);
#define isPortHigh(pin, pd) ((1 << pd) & pin) != 0

#define dcHighPin0 PD6
#define dcHighPin1 PD7
#define dcHighPin2 PB0
#define dcHighPin3 PB1

#define dcLowPin0 PB7
#define dcLowPin1 PD3
#define dcLowPin2 PD2
#define dcLowPin3 PB6

#define tubePin14 PC0
#define tubePin25 PC1
#define tubePin36 PC2

#define dpHighPin PD4
#define dpLowPin PC3

#define beepPin PD5
#define ledPin PB2

uint8_t EEMEM eeprom_bright = 3;
uint8_t EEMEM eeprom_ledState = 1;
uint8_t EEMEM eeprom_isHourBeepActive = 1;

uint8_t EEMEM eeprom_isAlarmActive = 0;
uint8_t EEMEM eeprom_alarmHour = 7;
uint8_t EEMEM eeprom_alarmMin = 0;

uint8_t EEMEM eeprom_nightThreshold = 25;

	
// Mode Work
#define mw_Clock 0

#define mw_SetSec 1
#define mw_SetMin 2
#define mw_SetHour 3

#define mw_SetIsAlarmActive 4
#define mw_SetAlarmMin 5
#define mw_SetAlarmHour 6

#define mw_SetYear 7
#define mw_SetMonth 8
#define mw_SetDay 9

#define mw_SetThrashhold 10

#define mw_LastMW mw_SetThrashhold

#define tm_Etching -1
#define tm_ShowTime 0
#define tm_ShowAlarm 1
#define tm_ShowDate 2
#define tm_ShowNightModeThrashhold 3

#define rank_null 0
#define rank_tube_12 1
#define rank_tube_34 2
#define rank_tube_56 3

struct DateTime {
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint8_t wday;
};

struct Beeper {
	bool isCanInit;
	bool canBeep;
	bool isPinActive;
	uint8_t	count;
	uint8_t	initCount;
	uint16_t durationActive;
	uint16_t durationInactive;
	uint16_t waitingTime;
	uint64_t beepTime;
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
	uint64_t ledTime;
};

struct Alarm {
	bool isActive;
	bool isTurnedOff;
	uint8_t lastDay;
	uint8_t startHour;
	uint8_t startMin;
	Beeper beeper;
};

struct Button {
	bool isLongPress;
	bool notInclude;
	uint8_t num;
	int16_t adc;
	uint64_t bounceTime;
	uint64_t longPressTime;
};

struct Bright {
	uint8_t level;
	uint8_t dayLevel;
};

struct Tube {
	bool isFlash[6];
	bool isDisabled[6];
	bool isDoteActive[6];
	uint8_t dc[4];
	uint8_t value[6];
	uint64_t switchTime;
};

struct NightMode {
	bool isActive;
	bool isCanTryActivate;
	uint8_t modeType;
	int16_t threshold;
	uint8_t thresholdInit;
	uint8_t hourStart;
	uint8_t hourStop;
	int16_t adc;
	uint64_t autoStamp;
};

struct TimeCorrection {
	bool isForward;
	uint8_t value;
	uint8_t interval;
	uint8_t year;
	uint64_t stamp;
};

struct EditValue {
	bool isGrabbed;
	int8_t value;
	int8_t max;
	int8_t min;
};

struct Etching {
	bool isWorking;
	uint8_t lastMin;
	uint8_t value;
	uint16_t duration;
	uint64_t switchTime;
};

struct ShowDate {
	uint8_t start;
	uint8_t stop;
	bool isActive;	
};

DateTime dateTime;
ShowDate showDate;
HourBeep hourBeep;
Alarm alarm;
Button button;
Bright bright;
Tube tube;
NightMode nightMode;
TimeCorrection correction;
EditValue editValue;
Etching etching;
LedBlinking ledBlinking;

bool isLedActive;

int8_t tubeMode;
uint8_t modeWork = mw_Clock;                                    
uint8_t rank;
uint8_t returnTime;
uint8_t prevSec;


int16_t getADC(uint8_t);
void tubeSwitch();
void resetCorrectionParams();
void buttonController();
void assignEditDigit();
void buttonShortPress();
void buttonLongPress();
void timeCorrection();
void init();
void firstButtonLongPress();
void resetButtons();
void resetButtonPress();
void setTimeMode();
void switchOffBeepValues();
void tubeAsMode();
uint8_t getMaxMonthDay();
void setTubeDC();
void setZeroCorrectionDate();
void setDayMode();
void checkNightMode();
void setNightMode();
void clockBeeper();
void setStartEtchingValues();
void eepromWriteByte(uint8_t,uint8_t);
uint8_t eepromReadByte(uint8_t);
uint64_t calcTimeStamp(uint8_t,uint8_t,uint8_t);
uint8_t ds1307_dec2bcd(uint8_t);
uint8_t ds1307_bcd2dec(uint8_t);
void ds1307_setdate(DateTime);
DateTime ds1307_getdate();
void showDissallowedTask();
void beepController(Beeper*);
