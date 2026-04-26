// ============================================================
// hardware.h — Project-specific board pin definitions
// 
// This is the ONLY file you need to modify when adapting
// the firmware to a different hardware board.
// ============================================================
#ifndef HARDWARE_H
#define HARDWARE_H

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>

// --- Port manipulation macros ---
#define switchPort(port, pin, onOff) \
	do { if (onOff) (port) |= _BV(pin); else (port) &= ~_BV(pin); } while(0)

#define isPortHigh(pinReg, pin) (((1 << (pin)) & (pinReg)) != 0)

// --- BCD decoder pins (high bank: tubes 1-3) ---
#define PIN_DECODER_HIGH_0   PD6
#define PIN_DECODER_HIGH_1   PD7
#define PIN_DECODER_HIGH_2   PB0
#define PIN_DECODER_HIGH_3   PB1

// --- BCD decoder pins (low bank: tubes 4-6) ---
#define PIN_DECODER_LOW_0    PB7
#define PIN_DECODER_LOW_1    PD3
#define PIN_DECODER_LOW_2    PD2
#define PIN_DECODER_LOW_3    PB6

// --- Anode control pins (pairs of tubes) ---
#define PIN_ANODE_PAIR_14    PC0
#define PIN_ANODE_PAIR_25    PC1
#define PIN_ANODE_PAIR_36    PC2

// --- Decimal point pins ---
#define PIN_DOT_HIGH         PD4
#define PIN_DOT_LOW          PC3

// --- Buzzer and LED ---
#define PIN_BUZZER           PD5
#define PIN_LED              PB2

// --- ADC channels ---
#define ADC_CHANNEL_BUTTONS  6
#define ADC_CHANNEL_LIGHT    7

// --- Data direction registers for this board ---
#define INIT_DDRD  0xFC
#define INIT_DDRB  0xC7
#define INIT_DDRC  0x0F

#endif // HARDWARE_H
