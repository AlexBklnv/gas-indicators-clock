#ifndef EEPROM_UTILS_H
#define EEPROM_UTILS_H

#include <avr/eeprom.h>
#include <stdint.h>

// Safe EEPROM read: returns stored value if <= maxVal,
// otherwise writes defaultVal and returns it.
// Protects against corrupted EEPROM data after first flash.
uint8_t eeprom_readSafe(uint8_t* addr, uint8_t defaultVal, uint8_t maxVal);

#endif // EEPROM_UTILS_H
