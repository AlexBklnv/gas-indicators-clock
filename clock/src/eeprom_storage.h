// ============================================================
// eeprom_storage.h — Project-specific EEPROM variable layout
//
// Each project defines its own set of EEPROM variables here.
// The reusable eeprom_readSafe() lives in lib/eeprom_utils/.
// ============================================================
#ifndef EEPROM_STORAGE_H
#define EEPROM_STORAGE_H

#include "eeprom_utils.h"

// --- Brightness ---
extern uint8_t EEMEM eeprom_bright;
extern uint8_t EEMEM eeprom_ledState;

// --- Hour beep ---
extern uint8_t EEMEM eeprom_isHourBeepActive;

// --- Alarm 1 ---
extern uint8_t EEMEM eeprom_isAlarm1Active;
extern uint8_t EEMEM eeprom_alarm1Hour;
extern uint8_t EEMEM eeprom_alarm1Min;

// --- Alarm 2 ---
extern uint8_t EEMEM eeprom_isAlarm2Active;
extern uint8_t EEMEM eeprom_alarm2Hour;
extern uint8_t EEMEM eeprom_alarm2Min;

// --- Time correction ---
extern uint8_t EEMEM eeprom_correctionIsForward;
extern uint8_t EEMEM eeprom_correctionValue;
extern uint8_t EEMEM eeprom_correctionInterval;
extern uint8_t EEMEM eeprom_correctionLastDay;
extern uint8_t EEMEM eeprom_correctionLastMonth;
extern uint8_t EEMEM eeprom_correctionLastYear;

// --- Show date ---
extern uint8_t EEMEM eeprom_showDateIsActive;
extern uint8_t EEMEM eeprom_showDateStart;
extern uint8_t EEMEM eeprom_showDateStop;

// --- Night mode ---
extern uint8_t EEMEM eeprom_hourNightModeStart;
extern uint8_t EEMEM eeprom_hourNightModeStop;
extern uint8_t EEMEM eeprom_nightMode;
extern uint8_t EEMEM eeprom_nightThreshold;

// --- Year offset for DS1307 post-2099 support ---
extern uint8_t EEMEM eeprom_yearOffset;

#endif // EEPROM_STORAGE_H
