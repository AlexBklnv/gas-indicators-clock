#include "eeprom_utils.h"

uint8_t eeprom_readSafe(uint8_t* addr, uint8_t defaultVal, uint8_t maxVal) {
	uint8_t value = eeprom_read_byte(addr);
	if (value > maxVal) {
		eeprom_update_byte(addr, defaultVal);
		return defaultVal;
	}
	return value;
}
