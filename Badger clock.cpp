#define F_CPU 8000000UL
#include <util/delay.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include "Lib/RTC/rtc.h"
#include "Lib/TWI/twi.h"
#include "Lib/millis.h"

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

DateTime dateTime;

bool isDotEtching = false;

bool isAlarmActive = true;
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
bool isNightModeActive = false;
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

int8_t bufByte = 1;
int8_t setDigit;

uint8_t hourBeepStart = 6;
uint8_t hourBeepStop = 23;
uint8_t hourNightModeStart = 21;
uint8_t hourNightModeStop = 6;
uint8_t firstStart=1;
uint8_t alarmMin = 1;
uint8_t alarmHour = 7;
uint8_t bright = 1;
uint8_t dayBrightTmp = 1;
uint8_t button;
uint8_t correctionTime = 0;
uint8_t dotCounter;
uint8_t etchingCounter;
uint8_t lastDay;
uint8_t lastEtchingMin;
uint8_t lastHour;
uint8_t lastSec;
uint8_t maxValueOfSetDigit;
uint8_t minValueOfSetDigit;
uint8_t modeWork = 0;
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
void setZeroDate();
void setDayMode();
void checkNightMode();
void setNightMode();

int main(void) {
	init();
	while (1) {
		mainCycle();
	}
}

void initStartDate() {
	lastSec = dateTime.sec;
	lastDay = dateTime.day;
	lastEtchingMin = dateTime.min;
	lastHour = dateTime.hour;
}

void init(){
	DDRD = 0xFC;
	DDRB = 0xC7;
	DDRC = 0x0F;
	ADMUX = (1 << REFS0);				 // Напряжение питания в качестве опорного
	ADCSRA |= (1<<ADEN)				 // Разрешение использования АЦП
	|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);	 // Делитель 128 = 64 кГц
	
	initMillis();
	twi_init_master();
	rtc_init();
	
	firstStart = eeprom_read_byte((uint8_t*)1);
	if (firstStart == 201) {
		bright = eeprom_read_byte((uint8_t*)2);
		isAlarmActive = eeprom_read_byte((uint8_t*)3);
		ledState = eeprom_read_byte((uint8_t*)4);
		isHourBeepActive = eeprom_read_byte((uint8_t*)5);
		isForwardCorrection = eeprom_read_byte((uint8_t*)6);
		correctionTime = eeprom_read_byte((uint8_t*)7);
		isCanShowDate =	eeprom_read_byte((uint8_t*)8);
		alarmHour = eeprom_read_byte((uint8_t*)9);
		alarmMin = eeprom_read_byte((uint8_t*)10);
		hourBeepStart = eeprom_read_byte((uint8_t*)11);
		hourBeepStop = eeprom_read_byte((uint8_t*)12);
		hourNightModeStart = eeprom_read_byte((uint8_t*)13);
		hourNightModeStop = eeprom_read_byte((uint8_t*)14);
		isNightModeActive = eeprom_read_byte((uint8_t*)15);
		} else {
		eeprom_write_byte((uint8_t*)2, bright);
		eeprom_write_byte((uint8_t*)3, isAlarmActive);
		eeprom_write_byte((uint8_t*)4, ledState);
		eeprom_write_byte((uint8_t*)5, isHourBeepActive);
		eeprom_write_byte((uint8_t*)6, isForwardCorrection);
		eeprom_write_byte((uint8_t*)7, correctionTime);
		eeprom_write_byte((uint8_t*)8, isCanShowDate);
		eeprom_write_byte((uint8_t*)9, alarmHour);
		eeprom_write_byte((uint8_t*)10, alarmMin);
		eeprom_write_byte((uint8_t*)11, hourBeepStart);
		eeprom_write_byte((uint8_t*)12, hourBeepStop);
		eeprom_write_byte((uint8_t*)13, hourNightModeStart);
		eeprom_write_byte((uint8_t*)14, hourNightModeStop);
		eeprom_write_byte((uint8_t*)15, isNightModeActive);
		rtc_run_clock(true);
	}
	
	switchPort(PORTB, ledPin, ledState);

	setTube5DP();
	setTube6DP();
	resetButtonPress();
	flashDownInit();
	
	dateTime = rtc_get_time();
	initStartDate();
}

void mainCycle(){
	dateTime = rtc_get_time();
	if (modeWork == 0) {
		if (lastSec != dateTime.sec && firstStart != 201) {
			dateTime.hour = 10;
			dateTime.min = 59;
			dateTime.sec = 50;
			dateTime.wday = 4;
			dateTime.day = 12;
			dateTime.month = 10;
			dateTime.year = 17;
			rtc_set_time(dateTime);
			firstStart = 201;
			eeprom_write_byte((uint8_t*)1, firstStart);
			initStartDate();
		}
		checkCorrectionTime();
		checkAlarmTime();
		if (lastHour != dateTime.hour) {
			lastHour = dateTime.hour;
			if (!isAlarmTime && isHourBeepActive && dateTime.sec < 3) {
				isHourBeepTime = true;
				isBeep1 = true;
			}
		}
	}
	if (!isEtchingCanStart || isAlarmTime) {
		if (modeWork == 0) {
			rank = 0;
			checkNightMode();
			showTimeMode();
			} else {
			switchOffBeepValues();
			if (lastSec != dateTime.sec) {
				lastSec = dateTime.sec;
				if (returnTime == 0) {
					modeWork = 0;
					resetButtonPress();
					flashDownInit();
					} else {
					returnTime--;
				}
			}
			if (isSetModeFirstTime) {
				assignSetDigit();
			}
		}
		tubeAsMode();
		buttonAnalyzer();
		setTubeDC(false);
		} else {
		button = 0;
		resetButtonPress();
		if (isDotEtching) {
			if (millis() - dotTime >= 30) {
				isTubeDPOn[dotCounter] = !isTubeDPOn[dotCounter];
				dotCounter = dotCounter != 6? dotCounter + 1: 0;
				dotTime = millis();
			}
		}
		if (millis() - buttonEtchingTime >= 200) {
			if (etchingCounter >= 10) {
				isEtchingCanStart = false;
				if (isDotEtching) {
					for (uint8_t i = 0; i < 6; i++) {
						isTubeDPOn[i] = false;
					}
				}
				} else {
				etchingCounter++;
				buttonEtchingTime = millis();
			}
		}
		setTubeDC(etchingCounter < 10 ? true : false);
	}
}

uint16_t readAdc(uint8_t channel) {
	ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCW;
}

void tubeSwitch() {
	
	switch(bright){
		case 0: _delay_us(1); break;
		case 1: _delay_us(266); break;
		case 2: _delay_us(533); break;
		case 3: _delay_us(800); break;
	}
	
	switchPort(PORTC, tubePin14, 1);
	switchPort(PORTC, tubePin25, 1);
	switchPort(PORTC, tubePin36, 1);
	
	if (modeWork > 0) {
		for (uint8_t i = 0; i < 6; i++) {
			if (isTubeFlash[i]) {
				if (millis() - tubeOnOffCounter >= 150) {
					isTubeOff[i] =  true;
					if (millis() - tubeOnOffCounter >= 550) {
						tubeOnOffCounter = millis() ;
					}
					} else {
					isTubeOff[i] = false;
				}
				} else {
				isTubeOff[i] = false;
			}
		}
	}
	
	_delay_us(800);
	
	switchPort(PORTB, dcLowPin0, 1);
	switchPort(PORTD, dcLowPin1, 1);
	switchPort(PORTD, dcLowPin2, 1);
	switchPort(PORTB, dcLowPin3, 1);
	switchPort(PORTD, dcHighPin0, 1);
	switchPort(PORTD, dcHighPin1, 1);
	switchPort(PORTB, dcHighPin2, 1);
	switchPort(PORTB, dcHighPin3, 1);
	switchPort(PORTC, dpLowPin, 0);
	switchPort(PORTD, dpHighPin, 0);
	_delay_us(400);
}

void translitDecoder(char tubeVal) {
	for (int i = 0; i < 4; i++) {
		dcTube[i] = 0;
	}
	for(int i = 3; tubeVal != 0; i--) {
		dcTube[i] = tubeVal % 2;
		tubeVal /= 2;
	}
}

void setTubeDC(bool isEtching){
	if (isEtching){
		translitDecoder(etchingCounter);
	}
	for(int i = 0; i < 3; i++){
		if (!isTubeOff[i]) {
			if (!isEtching){
				translitDecoder(tubeValue[i]);
			}
			switchPort(PORTD, dpHighPin, isTubeDPOn[i]);
			
			switchPort(PORTD, dcHighPin0, dcTube[3]);
			switchPort(PORTD, dcHighPin1, dcTube[2]);
			switchPort(PORTB, dcHighPin2, dcTube[1]);
			switchPort(PORTB, dcHighPin3, dcTube[0]);
		}
		if (!isTubeOff[i + 3]) {
			if (!isEtching){
				translitDecoder(tubeValue[i+3]);
			}
			switchPort(PORTC, dpLowPin, isTubeDPOn[i+3]);
			
			switchPort(PORTB, dcLowPin0, dcTube[3]);
			switchPort(PORTD, dcLowPin1, dcTube[2]);
			switchPort(PORTD, dcLowPin2, dcTube[1]);
			switchPort(PORTB, dcLowPin3, dcTube[0]);
		}
		switch(i){
			case 0: switchPort(PORTC, tubePin14, 0); break;
			case 1: switchPort(PORTC, tubePin25, 0); break;
			case 2: switchPort(PORTC, tubePin36, 0); break;
		}
		tubeSwitch();
	}
}

void checkNightMode() {
	if (isNightModeActive) {
		if (hourNightModeStart > hourNightModeStop){
			if (dateTime.hour >= hourNightModeStop && dateTime.hour < hourNightModeStart) {
				setDayMode();
			} else {
				setNightMode();
			}
		} else {
			if (dateTime.hour >= hourNightModeStart && dateTime.hour < hourNightModeStop) {
				setNightMode();
			} else {
				setDayMode();
			}
		}
	}
}

void setNightMode(){
	isNightTime = true;
	bright = 0;
	ledState = false;
	switchPort(PORTB,ledPin, ledState);
}

void setDayMode(){
	isNightTime = false;
	bright = dayBrightTmp;
	ledState = dayLedState;
	switchPort(PORTB,ledPin, ledState);
}



void checkAlarmTime() {
	if (isAlarmActive) {
		if (dateTime.hour == alarmHour) {
			if (dateTime.min == alarmMin) {
				if (!isAlarmTime) {
					isAlarmTime = true;
					isCalcBeepSec = true;
					lastSec = 59;
					isUserTurnOffAlarm = false;
				}
				} else {
				if (isAlarmTime) {
					isAlarmTime = false;
					isUserTurnOffAlarm = true;
					switchPort(PORTD,beepPin, isAlarmTime);
				}
			}
		}
	}
}

void checkCorrectionTime() {
	if (correctionTime > 0) {
		if (lastDay != dateTime.day) {
			if (dateTime.hour == 3 && dateTime.min >= 1) {
				if (!isForwardCorrection) {
					bufByte = dateTime.sec - correctionTime;
					if (bufByte >= 0) {
						dateTime.sec = bufByte;
						} else {
						dateTime.min--;
						dateTime.sec = 60 - abs(bufByte);
					}
					} else {
					bufByte = dateTime.sec + correctionTime;
					if (bufByte < 60) {
						dateTime.sec = bufByte;
						} else {
						dateTime.min++;
						dateTime.sec = bufByte - 60;
					}
				}
				lastSec = dateTime.sec;
				rtc_set_time(dateTime);
				setZeroDate();
				lastDay = dateTime.day;
			}
			} else {
			setZeroDate();
		}
	}
}


void flashDownInit() {
	for (uint8_t i = 0; i < 6; i++) {
		isTubeFlash[i] = false;
		isTubeOff[i] = false;
		isTubeDPOn[i] = false;
	}
}

void resetButtonPress(){
	isLongPress = false;
	isPressedButton1 = false;
	isPressedButton2 = false;
}

void buttonAnalyzer() {
	buttonADC = readAdc(6);
	if (buttonADC < 581) {
		button = 2;
		buttonDetectTime = millis();
		} else {
		if (buttonADC < 770) {
			button = 1;
			buttonDetectTime = millis();
			} else {
			button = 0;
			isButtonsFirstPress = false;
		}
	}
	
	if (!isButtonsFirstPress) {
		if (millis() - buttonDetectTime > 60) {
			isButtonsFirstPress = true;
		}
		} else {
		if (!isUserTurnOffAlarm) {
			isUserTurnOffAlarm = true;
			isButtonsFirstPress = false;
			} else {
			if (button == 1) {
				if (!isPressedButton1) {
					buttonEtchingTime = millis();
					isPressedButton1 = true;
					} else {
					buttonLongPress();
				}
				} else {
				if (button == 2) {
					if (!isPressedButton2) {
						buttonEtchingTime = millis();
						isPressedButton2 = true;
						} else {
						buttonLongPress();
					}
				}
			}
		}
	}
	
	if (isPressedButton1 || isPressedButton2) {
		if (button == 0) {
			if (!isLongPress) {
				returnTime = 15;
				isButtonsFirstPress = false;
				buttonEtchingTime = millis();
				buttonShortPress();
			}
			resetButtonPress();
		}
	}
}

void buttonLongPress(){
	if (millis() - buttonEtchingTime > 1300) {
		isLongPress = true;
		returnTime = 15;
		if (isPressedButton1) {
			firstButtonLongPress();
			isPressedButton1 = false;
			} else {
			if (isPressedButton2) {
				if (modeWork == 0) {
					ledState = !ledState;
					switchPort(PORTB,ledPin, ledState);
					if (!isNightTime) {
						dayLedState = ledState;
						eeprom_write_byte((uint8_t*)4, ledState);
					}
				}
			}
			isPressedButton2 = false;
		}
	}
}

void buttonShortPress() {
	if (modeWork == 0) {
		if (isPressedButton1) {
			if (!isHourBeepTime) {
				isHourBeepActive = !isHourBeepActive;
				isHourBeepTime = isHourBeepActive;
				isBeep1 = isHourBeepActive;
				isUserTurnOnHourBeep = isHourBeepActive;
				eeprom_write_byte((uint8_t*)5, isHourBeepActive);
			}
		}
		if (isPressedButton2) {
			bright = bright >= 3? 0: bright+1;
			if (!isNightTime) {
				dayBrightTmp = bright;
				eeprom_write_byte((uint8_t*)2, bright);
			}
		}
		
		} else if (modeWork == 1) {
		if (dateTime.sec >= 30) {
			if (dateTime.min == 59) {
				dateTime.min = 0;
				if (dateTime.hour == 23) {
					dateTime.hour = 0;
					if (dateTime.day ==  getMaxMounthDay()) {
						dateTime.day = 1;
						if (dateTime.month == 12) {
							dateTime.month = 1;
							if (dateTime.year == 41) {
								dateTime.year = 17;
								} else {
								dateTime.year++;
							}
							} else {
							dateTime.month++;
						}
						} else {
						dateTime.day++;
					}
					lastDay = dateTime.day;
					} else {
					dateTime.hour++;
				}
				lastHour = dateTime.hour;
				} else {
				dateTime.min++;
			}
			lastEtchingMin = dateTime.min;
		}
		dateTime.sec = 0;
		setDigit = 0;
		lastSec = 0;
		rtc_set_time(dateTime);
		} else if (modeWork > 1 && modeWork < 17) {
			if (modeWork == 7) {
				isAlarmActive = !isAlarmActive;
				setTube5DP();
				setDigit = isAlarmActive;
			} else {
				if (modeWork == 10) {
					isForwardCorrection = isForwardCorrection ? false: true;
					setDigit = isForwardCorrection ? 1: 0;
				} else if (modeWork == 14) {
					isNightModeActive = !isNightModeActive;
					setDigit = isNightModeActive ? 1: 0;
				} else {
					if (isPressedButton1) {
						setDigit++;
					} else {
						if (isPressedButton2) {
							setDigit--;
					}
				}
			}
		}
		if (setDigit > maxValueOfSetDigit) {
			setDigit = minValueOfSetDigit;
			} else if (setDigit < minValueOfSetDigit) {
			setDigit = maxValueOfSetDigit;
		}
	}
}

void firstButtonLongPress() {
	if (modeWork > 1 && modeWork < 4) {
		lastDay = dateTime.day;
	}
	switch(modeWork) {
		case 2:
			dateTime.min = setDigit;
			lastEtchingMin = dateTime.min;
			break;
		case 3:
			dateTime.hour = setDigit;
			lastHour = dateTime.hour;
			break;
		case 4:  
			dateTime.year = setDigit; 
			break;
		case 5: 
			dateTime.month = setDigit;
			break;
		case 6:
			dateTime.day = setDigit;
			isCanShowDate = dateTime.day == 0 ? false : true;
			break;
		case 7:
			eeprom_write_byte((uint8_t*)3, isAlarmActive);
			break;
		case 8:
			alarmMin = setDigit;
			eeprom_write_byte((uint8_t*)10, alarmMin);
			break;
		case 9:
			alarmHour = setDigit;
			eeprom_write_byte((uint8_t*)9, alarmHour);
			break;
		case 10:
			lastDay = dateTime.day;
			eeprom_write_byte((uint8_t*)6, isForwardCorrection);
			break;
		case 11:
			correctionTime = setDigit;
			eeprom_write_byte((uint8_t*)7, correctionTime);
			break;
		case 12:
			hourBeepStop = setDigit == 0? 24 : setDigit;
			eeprom_write_byte((uint8_t*)12, hourBeepStop);
			break;
		case 13:
			hourBeepStart = setDigit;
			eeprom_write_byte((uint8_t*)11, hourBeepStart);
			break;
		case 14:
			eeprom_write_byte((uint8_t*)15, isNightModeActive);
			break;
		case 15:
			hourNightModeStop = setDigit == 0? 24 : setDigit;
			eeprom_write_byte((uint8_t*)14, hourNightModeStop);
			break;
		case 16:
			hourNightModeStart = setDigit;
			eeprom_write_byte((uint8_t*)13, hourNightModeStart);
			break;
	}
	
	if (modeWork > 1 && modeWork < 7) {
		rtc_set_time(dateTime);
	}
	
	modeWork = modeWork < 16? modeWork + 1 : 0;
	flashDownInit();
	isSetModeFirstTime = true;
}

void setStartEtchingValues() {
	lastEtchingMin = dateTime.min;
	isEtchingCanStart = true;
	etchingCounter = 0;
	dotCounter = 0;
	if (isDotEtching) {
		for (int i = 0; i < 6; i++) {
			isTubeDPOn[i] = false;
		}
		dotTime = millis();
	}
	buttonEtchingTime = millis();
}

void switchOffBeepValues() {
	switchPort(PORTD, beepPin, 0);
	isHourBeepTime = false;
	isBeep2 = false;
	isUserTurnOnHourBeep = false;
}

void clockBeeper() {
	if (isBeep1) {
		switchPort(PORTD, beepPin, 1);
		beepTime = millis();
		isBeep1 = false;
		isBeep2 = true;
	}
	if (millis() - beepTime > 100) {
		if (isBeep2 && isPortHigh(PIND,beepPin)) {
			switchPort(PORTD, beepPin, 0);
			beepTime = millis();
			} else {
			if (isBeep2) {
				switchPort(PORTD, beepPin, 1);
				isBeep2 = false;
				beepTime = millis();
				} else {
				isCalcBeepSec = true;
				lastSec = dateTime.sec;
				switchOffBeepValues();
			}
		}
	}
}


void showTimeMode() {
	if (isAlarmTime) {
		if (!isUserTurnOffAlarm) {
			if (isCalcBeepSec) {
				if (abs(lastSec - dateTime.sec) > 0) {
					isBeep1 = true;
					isCalcBeepSec = false;
				}
			} else {
				clockBeeper();
			}
		} else {
			switchPort(PORTD, beepPin, 0);
		}
	} else {
		if ((isHourBeepActive && isHourBeepTime) || isUserTurnOnHourBeep) {
			if (isUserTurnOnHourBeep) {
				clockBeeper();
			} else {
				if (hourBeepStart > hourBeepStop){
					if (dateTime.hour > hourBeepStop && dateTime.hour < hourBeepStart) {
						
						} else {
						clockBeeper();
					}
					} else {
					if (dateTime.hour >= hourBeepStart && dateTime.hour <= hourBeepStop) {
						clockBeeper();
					}
					}
				}
		}
	}
	setTube5DP();
	setTube6DP();
	if (lastEtchingMin != dateTime.min) {
		if (dateTime.sec < 3) {
			if (dateTime.min == 0 && isHourBeepActive) {
				if (!isHourBeepTime) {
					setStartEtchingValues();
				}
				} else {
				if (dateTime.min % 10 == 0) {
					setStartEtchingValues();
				}
			}
		}
	}
	
	tubeMode = dateTime.sec > 50 && dateTime.sec < 54 
			&& isCanShowDate ? 1 : 0;
}

void setTube5DP() {
	isTubeDPOn[4] = isAlarmActive;
}

void setTube6DP(){
	isTubeDPOn[5] = isHourBeepActive;
}

void setRankLimit(uint8_t upper, uint8_t lower) {
	maxValueOfSetDigit = upper;
	minValueOfSetDigit = lower;
}

void setZeroDate() {
	if (!isCanShowDate) {
		dateTime.day = 0;
		rtc_set_time(dateTime);
	}
}

uint8_t getMaxMounthDay() {
	if (dateTime.month == 2) {
		return dateTime.year % 4 == 0 ? 29 : 28;
		} else {
		return dateTime.month % 2 == 0? 30 : 31;
	}
}

void assignSetDigit() {
	if (modeWork < 4) {
		tubeMode = 0;
		rank = 4 - modeWork;
		} else if (modeWork < 7) {
		tubeMode = 1;
		rank = 7 - modeWork;
		} else if (modeWork < 10) {
		tubeMode = 2;
		rank = 10 - modeWork;
		} else if (modeWork < 12) {
		tubeMode = 3;
		rank = 13 - modeWork;
		} else if (modeWork < 14) {
		tubeMode = 4;
		rank = 14 - modeWork;
		} else if (modeWork < 17) {
		tubeMode = 5;
		rank = 17 - modeWork;
	}
	isSetModeFirstTime = false;
	bufByte = rank * 2;
	for (uint8_t i = bufByte - 2; i < bufByte; i++) {
		isTubeFlash[i] = true;
	}
	
	if (tubeMode == 0) {
		switch(rank) {
			case 1:
			setDigit = dateTime.hour;
			setRankLimit(23, 0);
			break;
			case 2:
			setDigit = dateTime.min;
			setRankLimit(59, 0);
			break;
		}
	} else if (tubeMode == 1) {
		switch(rank) {
			case 1:
			setDigit = dateTime.day;
			setRankLimit(getMaxMounthDay(), 0);
			break;
			case 2:
			setDigit = dateTime.month;
			setRankLimit(12, 1);
			break;
			case 3:
			setDigit = dateTime.year;
			setRankLimit(41, 17);
			break;
		}
	} else if (tubeMode == 2) {
		setTube5DP();
		switch(rank) {
			case 1:
			setDigit = alarmHour;
			setRankLimit(23, 0);
			break;
			case 2:
			setDigit = alarmMin;
			setRankLimit(59, 0);
			break;
			case 3: setDigit = isAlarmActive; break;
		}
	} else if (tubeMode == 3) {
		correctionTime = eeprom_read_byte((uint8_t*)7);
		switch(rank) {
			case 2: setDigit = correctionTime; break;
			case 3: setDigit = isForwardCorrection; break;
	}
	} else if (tubeMode == 4) {
	switch(rank) {
		case 1:
		setDigit = hourBeepStart;
		break;
		case 2:
		setDigit = hourBeepStop == 24? 0: hourBeepStop;
		break;
	}
	setRankLimit(23, 0);
	}  else if (tubeMode == 5) {
	switch(rank) {
		case 1:
		setDigit = hourNightModeStart;
		setRankLimit(23, 0);
		break;
		case 2:
		setDigit = hourNightModeStop == 24? 0: hourNightModeStop;
		setRankLimit(23, 0);
		break;
		case 3: setDigit = isNightModeActive; break;
	}
	}
}

void fillTubeValueFromIndex(uint8_t startIndex, uint8_t value) {
	startIndex = startIndex * 2 - 2;
	tubeValue[startIndex] = value < 10? 0 : value / 10;
	tubeValue[startIndex + 1] = value < 10? value : value % 10;
}

void tubeAsMode() {
	if (tubeMode == 0) {
		fillTubeValueFromIndex(1, dateTime.hour);
		fillTubeValueFromIndex(2, dateTime.min);
		fillTubeValueFromIndex(3, dateTime.sec);
	} else if (tubeMode == 1) {
		fillTubeValueFromIndex(1, dateTime.day);
		fillTubeValueFromIndex(2, dateTime.month);
		fillTubeValueFromIndex(3, dateTime.year);
	} else if (tubeMode == 2) {
		fillTubeValueFromIndex(1, alarmHour);
		fillTubeValueFromIndex(2, alarmMin);
		fillTubeValueFromIndex(3, isAlarmActive);
		setTube5DP();
	} else if (tubeMode == 3) {
		fillTubeValueFromIndex(1, 0);
		fillTubeValueFromIndex(2, correctionTime);
		fillTubeValueFromIndex(3, isForwardCorrection);
	} else if (tubeMode == 4) {
		fillTubeValueFromIndex(1, hourBeepStart);
		fillTubeValueFromIndex(2, hourBeepStop);
		fillTubeValueFromIndex(3, 0);
	} else if (tubeMode == 5) {
		fillTubeValueFromIndex(1, hourNightModeStart);
		fillTubeValueFromIndex(2, hourNightModeStop);
		fillTubeValueFromIndex(3, isNightModeActive);
	}
	
	if (rank != 0) {
		if (tubeMode == 0 && rank == 3) {
			setDigit = dateTime.sec;
		}
		for (uint8_t k = 1; k < 4; k++) {
			if (rank == k) {
				fillTubeValueFromIndex(rank, setDigit);
				break;
			}
		}
	}
}
