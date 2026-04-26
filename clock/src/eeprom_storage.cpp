// ============================================================
// eeprom_storage.cpp — Project-specific EEPROM defaults
// ============================================================
#include "eeprom_storage.h"

uint8_t EEMEM eeprom_bright = 3;
uint8_t EEMEM eeprom_ledState = 1;
uint8_t EEMEM eeprom_isHourBeepActive = 1;

uint8_t EEMEM eeprom_isAlarm1Active = 0;
uint8_t EEMEM eeprom_alarm1Hour = 7;
uint8_t EEMEM eeprom_alarm1Min = 0;

uint8_t EEMEM eeprom_isAlarm2Active = 0;
uint8_t EEMEM eeprom_alarm2Hour = 8;
uint8_t EEMEM eeprom_alarm2Min = 0;

uint8_t EEMEM eeprom_correctionIsForward = 1;
uint8_t EEMEM eeprom_correctionValue = 0;
uint8_t EEMEM eeprom_correctionInterval = 0;
uint8_t EEMEM eeprom_correctionLastDay = 0;
uint8_t EEMEM eeprom_correctionLastMonth = 0;
uint8_t EEMEM eeprom_correctionLastYear = 0;

uint8_t EEMEM eeprom_showDateIsActive = 1;
uint8_t EEMEM eeprom_showDateStart = 51;
uint8_t EEMEM eeprom_showDateStop = 53;

uint8_t EEMEM eeprom_hourNightModeStart = 22;
uint8_t EEMEM eeprom_hourNightModeStop = 7;
uint8_t EEMEM eeprom_nightMode = 2;
uint8_t EEMEM eeprom_nightThreshold = 25;

uint8_t EEMEM eeprom_yearOffset = 0;
