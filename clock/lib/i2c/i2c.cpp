#include "i2c.h"
#include <inttypes.h>
#include <compat/twi.h>

void i2c_init(void) {
	TWSR = 0;
	TWBR = 32;  // ~100 kHz I2C at 8 MHz F_CPU
}

unsigned char i2c_start(unsigned char address) {
	uint8_t status;
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	status = TW_STATUS & 0xF8;
	if ((status != TW_START) && (status != TW_REP_START)) return 1;
	TWDR = address;
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	status = TW_STATUS & 0xF8;
	if ((status != TW_MT_SLA_ACK) && (status != TW_MR_SLA_ACK)) return 1;
	return 0;
}

void i2c_start_wait(unsigned char address) {
	uint8_t status;
	uint16_t retries = 1000;
	while (retries--) {
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
		while (!(TWCR & (1<<TWINT)));
		status = TW_STATUS & 0xF8;
		if ((status != TW_START) && (status != TW_REP_START)) continue;
		TWDR = address;
		TWCR = (1<<TWINT) | (1<<TWEN);
		while (!(TWCR & (1<<TWINT)));
		status = TW_STATUS & 0xF8;
		if ((status == TW_MT_SLA_NACK) || (status == TW_MR_DATA_NACK)) {
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
			while (TWCR & (1<<TWSTO));
			continue;
		}
		break;
	}
}

unsigned char i2c_rep_start(unsigned char address) {
	return i2c_start(address);
}

void i2c_stop(void) {
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	while (TWCR & (1<<TWSTO));
}

unsigned char i2c_write(unsigned char data) {
	uint8_t status;
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	status = TW_STATUS & 0xF8;
	if (status != TW_MT_DATA_ACK) return 1;
	return 0;
}

unsigned char i2c_readAck(void) {
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	while (!(TWCR & (1<<TWINT)));
	return TWDR;
}

unsigned char i2c_readNak(void) {
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	return TWDR;
}
