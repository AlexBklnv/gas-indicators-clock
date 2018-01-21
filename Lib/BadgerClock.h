#define F_CPU 8000000UL
#include <util/delay.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include "RTC/rtc.h"
#include "TWI/twi.h"
#include "millis.h"

#define switchPort(port,pd, onOff) if (onOff == 1) port|= _BV(pd); else port&=~_BV(pd);
#define isPortHigh(pin, pd) (((1 << pd) & pin) != 0)

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

#define eeprom_address_firstStart 1
#define eeprom_address_bright 2
#define eeprom_address_isAlarmActive 3
#define eeprom_address_ledState 4
#define eeprom_address_isHourBeepActive 5
#define eeprom_address_isForwardCorrection 6
#define eeprom_address_correctionTime 7
#define eeprom_address_isCanShowDate 8
#define eeprom_address_alarmHour 9
#define eeprom_address_alarmMin 10
#define eeprom_address_hourBeepStart 11
#define eeprom_address_hourBeepStop 12
#define eeprom_address_hourNightModeStart 13
#define eeprom_address_hourNightModeStop 14
#define eeprom_address_isNightModeActive 15
#define eeprom_address_daysBeforeCorrection 16

// Mode Work
#define mw_Clock 0
#define mw_SetSec 1
#define mw_SetMin 2
#define mw_SetHour 3
#define mw_SetYear 4
#define mw_SetMonth 5
#define mw_SetDay 6
#define mw_SetIsAlarmActive 7
#define mw_SetAlarmMin 8
#define mw_SetAlarmHour 9
#define mw_SetIsForwardCorrection 10
#define mw_SetCorrectionTime 11
#define mw_SetDaysBeforeCorrection 12
#define mw_SetIsHourBeepActive 13
#define mw_SetHourBeepStop 14
#define mw_SetHourBeepStart 15
#define mw_SetIsNightModeActive 16
#define mw_SetHourNightModeStop 17
#define mw_SetHourNightModeStart 18
#define mw_LastMW mw_SetHourNightModeStart

#define tm_ShowTime 0
#define tm_ShowDate 1
#define tm_ShowAlarm 2
#define tm_ShowCorrection 3
#define tm_ShowHourBeep 4
#define tm_ShowNightMode 5

#define rank_null 0
#define rank_tube_12 1
#define rank_tube_34 2
#define rank_tube_56 3

DateTime dateTime;

bool isDotEtching = false;

bool isAlarmActive = false;
bool isAlarmTime = false;
bool isBeep1;
bool isBeep2;
bool isButtonsFirstPress;
bool isCalcBeepSec;
bool isEtchingCanStart = false;
bool isForwardCorrection = true;
bool isHourBeepActive = true;
bool isHourBeepTime;
bool isLongPress;
bool isNightTime = false;
bool isNightModeActive = true;
bool isPressedButton1;
bool isPressedButton2;
bool isSetModeFirstTime = true;
bool isCanShowDate = true;
bool isTubeDPOn[6];
bool isTubeFlash[6];
bool isTubeOff[6];
bool isUserTurnOnHourBeep;
bool isUserTurnOffAlarm = true;
bool ledState = true;
bool dayLedState = true;

int8_t setDigit;

uint8_t hourBeepStart = 8;
uint8_t hourBeepStop = 22;
uint8_t hourNightModeStart = 20;
uint8_t hourNightModeStop = 8;
uint8_t firstStart=1;
uint8_t alarmMin = 1;
uint8_t alarmHour = 7;
uint8_t bright = 1;
uint8_t dayBrightTmp = 1;
uint8_t button;
uint8_t correctionTime = 0;
uint8_t daysBeforeCorrection = 0;
uint8_t daysWithoutCorrection = 0;
uint8_t dotCounter;
uint8_t etchingCounter;
uint8_t lastDay;
uint8_t lastEtchingMin;
uint8_t lastHour;
uint8_t lastSec;
uint8_t maxValueOfSetDigit;
uint8_t minValueOfSetDigit;
uint8_t modeWork = mw_Clock;
uint8_t tubeMode;                                       // 0-time | 1-date | 2-Alarm | 3-Correction
uint8_t rank;
uint8_t returnTime;

uint32_t buttonADC;

uint64_t beepTime;
uint64_t buttonDetectTime;
uint64_t buttonEtchingTime;
uint64_t dotTime;
uint64_t tubeOnOffCounter;

uint8_t dcTube[4];
uint8_t tubeValue[6];

void assignSetDigit();
void buttonAnalyzer();
void buttonShortPress();
void buttonLongPress();
void checkAlarmTime();
void checkCorrectionTime();
void init();
void firstButtonLongPress();
void flashDownInit();
void mainCycle();
void resetButtonPress();
void showTimeMode();
void switchOffBeepValues();
void tubeAsMode();
uint8_t getMaxMounthDay();
void setTubeDC(bool);
void setTube5DP();
void setTube6DP();
void setZeroCorrectionDate();
void setDayMode();
void checkNightMode();
void setNightMode();
void eepromWriteByte(uint8_t,uint8_t);
uint8_t eepromReadByte(uint8_t);
