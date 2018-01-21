#include "Lib/BadgerClock.h"

int main(void) {
	init();
	while (true) {
		mainCycle();
	}
}

void initStartDate() {
	lastSec = dateTime.sec;
	setZeroCorrectionDate();
	lastEtchingMin = dateTime.min;
	lastHour = dateTime.hour;
}

void init(){
	DDRD = 0xFC;
	DDRB = 0xC7;
	DDRC = 0x0F;
	ADMUX = (1 << REFS0);				 
	ADCSRA |= (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); 
	
	initMillis();
	twi_init_master();
	rtc_init();
	
	firstStart = eepromReadByte(eeprom_address_firstStart);
	if (firstStart == 201) {
		bright = eepromReadByte(eeprom_address_bright);
		isAlarmActive = eepromReadByte(eeprom_address_isAlarmActive);
		ledState = eepromReadByte(eeprom_address_ledState);
		isHourBeepActive = eepromReadByte(eeprom_address_isHourBeepActive);
		isForwardCorrection = eepromReadByte(eeprom_address_isForwardCorrection);
		correctionTime = eepromReadByte(eeprom_address_correctionTime);
		isCanShowDate =	eepromReadByte(eeprom_address_isCanShowDate);
		alarmHour = eepromReadByte(eeprom_address_alarmHour);
		alarmMin = eepromReadByte(eeprom_address_alarmMin);
		hourBeepStart = eepromReadByte(eeprom_address_hourBeepStart);
		hourBeepStop = eepromReadByte(eeprom_address_hourBeepStop);
		hourNightModeStart = eepromReadByte(eeprom_address_hourNightModeStart);
		hourNightModeStop = eepromReadByte(eeprom_address_hourNightModeStop);
		isNightModeActive = eepromReadByte(eeprom_address_isNightModeActive);
		daysBeforeCorrection = eepromReadByte(eeprom_address_daysBeforeCorrection);
	} else {
		eepromWriteByte(eeprom_address_bright, bright);
		eepromWriteByte(eeprom_address_isAlarmActive, isAlarmActive);
		eepromWriteByte(eeprom_address_ledState, ledState);
		eepromWriteByte(eeprom_address_isHourBeepActive, isHourBeepActive);
		eepromWriteByte(eeprom_address_isForwardCorrection, isForwardCorrection);
		eepromWriteByte(eeprom_address_correctionTime, correctionTime);
		eepromWriteByte(eeprom_address_isCanShowDate, isCanShowDate);
		eepromWriteByte(eeprom_address_alarmHour, alarmHour);
		eepromWriteByte(eeprom_address_alarmMin, alarmMin);
		eepromWriteByte(eeprom_address_hourBeepStart, hourBeepStart);
		eepromWriteByte(eeprom_address_hourBeepStop, hourBeepStop);
		eepromWriteByte(eeprom_address_hourNightModeStart, hourNightModeStart);
		eepromWriteByte(eeprom_address_hourNightModeStop, hourNightModeStop);
		eepromWriteByte(eeprom_address_isNightModeActive, isNightModeActive);
		eepromWriteByte(eeprom_address_daysBeforeCorrection, daysBeforeCorrection);
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
	if (modeWork == mw_Clock) {
		if (lastSec != dateTime.sec && firstStart != 201) {
			dateTime.hour = 19;
			dateTime.min = 15;
			dateTime.sec = 50;
			dateTime.wday = 5;
			dateTime.day = 20;
			dateTime.month = 1;
			dateTime.year = 18;
			rtc_set_time(dateTime);
			firstStart = 201;
			eepromWriteByte(eeprom_address_firstStart, firstStart);
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
		if (modeWork == mw_Clock) {
			rank = rank_null;
			checkNightMode();
			showTimeMode();
		} else {
			switchOffBeepValues();
			if (lastSec != dateTime.sec) {
				lastSec = dateTime.sec;
				if (returnTime == 0) {
					modeWork = mw_Clock;
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
	
	if (modeWork > mw_Clock) {
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
		if (hourNightModeStart == hourNightModeStop) {
			setNightMode();
			return;
		} else if (hourNightModeStart > hourNightModeStop){
			if (!(dateTime.hour > hourNightModeStop && dateTime.hour <= hourNightModeStart)){
				setNightMode();
				return;
			}
		} else {
			if (dateTime.hour >= hourNightModeStart && dateTime.hour < hourNightModeStop) {
				setNightMode();
				return;
			} 
		}
		setDayMode();
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
			daysWithoutCorrection++;
			if (!isCanShowDate) {
				dateTime.day = 0;
			}
			lastDay = dateTime.day;
			rtc_set_time(dateTime);
		} else if (daysWithoutCorrection ==  daysBeforeCorrection){
			if (dateTime.hour >= 3 && dateTime.min >= 3) {
				uint16_t buf = dateTime.min * 60 + dateTime.sec;
				if(!isForwardCorrection){
					buf -= correctionTime;
				} else {
					buf += correctionTime;
				}
				dateTime.min = buf / 60;
				dateTime.sec = buf % 60;
				lastSec = dateTime.sec;
				
				rtc_set_time(dateTime);
				setZeroCorrectionDate();
			}
		}
	}
}

void setZeroCorrectionDate() {
	lastDay = dateTime.day;
	daysWithoutCorrection = 0;
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
				if (modeWork == mw_Clock) {
					ledState = !ledState;
					switchPort(PORTB,ledPin, ledState);
					if (!isNightTime) {
						dayLedState = ledState;
						eepromWriteByte(eeprom_address_ledState, ledState);
					}
				}
			}
			isPressedButton2 = false;
		}
	}
}

void ActivateHourBeepByButton(bool status){
	isHourBeepActive = status;
	isHourBeepTime = isHourBeepActive;
	isBeep1 = isHourBeepActive;
	isUserTurnOnHourBeep = isHourBeepActive;
	eepromWriteByte(eeprom_address_isHourBeepActive, isHourBeepActive);
}

void buttonShortPress() {
	if (modeWork == mw_Clock) {
		if (isPressedButton1) {
			if (!isHourBeepTime) {
				ActivateHourBeepByButton(!isHourBeepActive);
			}
		}
		if (isPressedButton2) {
			bright = bright >= 3? 0: bright + 1;
			if (!isNightTime) {
				dayBrightTmp = bright;
				eepromWriteByte(eeprom_address_bright, bright);
			}
		}
	} else if (modeWork == mw_SetSec) {
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
					setZeroCorrectionDate();
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
	} else {			
			if (isPressedButton1) {
				setDigit++;
			} else if (isPressedButton2) {
				setDigit--;
			}

			if (setDigit > maxValueOfSetDigit) {
				setDigit = minValueOfSetDigit;
			} else if (setDigit < minValueOfSetDigit) {
				setDigit = maxValueOfSetDigit;
			}
	}
}

void firstButtonLongPress() {
	switch(modeWork) {
		case mw_SetMin:
			dateTime.min = setDigit;
			lastEtchingMin = dateTime.min;
			break;
		case mw_SetHour:
			dateTime.hour = setDigit;
			lastHour = dateTime.hour;
			break;
		case mw_SetYear:  
			dateTime.year = setDigit; 
			break;
		case mw_SetMonth: 
			dateTime.month = setDigit;
			break;
		case mw_SetDay:
			dateTime.day = setDigit;
			isCanShowDate = dateTime.day;
			break;
		case mw_SetIsAlarmActive:
			isAlarmActive = setDigit;
			eepromWriteByte(eeprom_address_isAlarmActive, isAlarmActive);
			break;
		case mw_SetAlarmMin:
			alarmMin = setDigit;
			eepromWriteByte(eeprom_address_alarmMin, alarmMin);
			break;
		case mw_SetAlarmHour:
			alarmHour = setDigit;
			eepromWriteByte(eeprom_address_alarmHour, alarmHour);
			break;
		case mw_SetIsForwardCorrection:
			isForwardCorrection = setDigit;
			eepromWriteByte(eeprom_address_isForwardCorrection, isForwardCorrection);
			break;
		case mw_SetCorrectionTime:
			correctionTime = setDigit;
			eepromWriteByte(eeprom_address_correctionTime, correctionTime);
			break;
		case mw_SetDaysBeforeCorrection:
			daysBeforeCorrection = setDigit;
			eepromWriteByte(eeprom_address_daysBeforeCorrection, daysBeforeCorrection);
			break;
		case mw_SetIsHourBeepActive:
			ActivateHourBeepByButton(setDigit);
			break;
		case mw_SetHourBeepStop:
			hourBeepStop = setDigit;
			eepromWriteByte(eeprom_address_hourBeepStop, hourBeepStop);
			break;
		case mw_SetHourBeepStart:
			 hourBeepStart = setDigit;
			eepromWriteByte(eeprom_address_hourBeepStart, hourBeepStart);
			break;
		case mw_SetIsNightModeActive:
			isNightModeActive = setDigit;
			eepromWriteByte(eeprom_address_isNightModeActive, isNightModeActive);
			break;
		case mw_SetHourNightModeStop:
			hourNightModeStop = setDigit;
			eepromWriteByte(eeprom_address_hourNightModeStop, hourNightModeStop);
			break;
		case mw_SetHourNightModeStart:
			hourNightModeStart = setDigit;
			eepromWriteByte(eeprom_address_hourNightModeStart, hourNightModeStart);
			break;
	}
	
	if (modeWork >= mw_SetMin && modeWork <= mw_SetDay) {
		rtc_set_time(dateTime);
	}
	if ((modeWork >= mw_SetSec && modeWork <= mw_SetDay)
		|| (modeWork >= mw_SetIsForwardCorrection && modeWork <= mw_SetDaysBeforeCorrection)) {
		setZeroCorrectionDate();
	}
	modeWork = modeWork < mw_LastMW? modeWork + 1 : 0;
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
					if (!(dateTime.hour > hourBeepStop && dateTime.hour < hourBeepStart)) {
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
			&& isCanShowDate ? tm_ShowDate : tm_ShowTime;
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

uint8_t getMaxMounthDay() {
	if (dateTime.month == 2) {
		return dateTime.year % 4 == 0 ? 29 : 28;
		} else {
		return dateTime.month % 2 == 0? 30 : 31;
	}
}

void assignSetDigit() {

	if (modeWork <= mw_SetHour) {
		tubeMode = tm_ShowTime;
		rank = mw_SetHour;
	} else if (modeWork <= mw_SetDay) {
		tubeMode = tm_ShowDate;
		rank = mw_SetDay;
	} else if (modeWork <= mw_SetAlarmHour) {
		tubeMode = tm_ShowAlarm;
		rank = mw_SetAlarmHour;
	} else if (modeWork <= mw_SetDaysBeforeCorrection) {
		tubeMode = tm_ShowCorrection;
		rank = mw_SetDaysBeforeCorrection;
	} else if (modeWork <= mw_SetHourBeepStart) {
		tubeMode = tm_ShowHourBeep;
		rank = mw_SetHourBeepStart;
	} else if (modeWork <= mw_SetHourNightModeStart) {
		tubeMode = tm_ShowNightMode;
		rank = mw_SetHourNightModeStart;
	}
	rank -= modeWork - 1;
	
	isSetModeFirstTime = false;
	int8_t bufByte = rank * 2;
	for (uint8_t i = bufByte - 2; i < bufByte; i++) {
		isTubeFlash[i] = true;
	}
	
	if (tubeMode == tm_ShowTime) {
		switch(rank) {
			case rank_tube_12:
				setDigit = dateTime.hour;
				setRankLimit(23, 0);
				break;
			case rank_tube_34:
				setDigit = dateTime.min;
				setRankLimit(59, 0);
				break;
		}
	} else if (tubeMode == tm_ShowDate) {
		switch(rank) {
			case rank_tube_12:
				setDigit = dateTime.day;
				setRankLimit(getMaxMounthDay(), 0);
				break;
			case rank_tube_34:
				setDigit = dateTime.month;
				setRankLimit(12, 1);
				break;
			case rank_tube_56:
				setDigit = dateTime.year;
				setRankLimit(41, 17);
				break;
		}
	} else if (tubeMode == tm_ShowAlarm) {
		setTube5DP();
		switch(rank) {
			case rank_tube_12:
				setDigit = alarmHour;
				setRankLimit(23, 0);
				break;
			case rank_tube_34:
				setDigit = alarmMin;
				setRankLimit(59, 0);
				break;
			case rank_tube_56: 
				setDigit = isAlarmActive; 
				setRankLimit(1, 0);
				break;
		}
	} else if (tubeMode == tm_ShowCorrection) {
		correctionTime = eepromReadByte(eeprom_address_correctionTime);
		switch(rank) {
			case rank_tube_12:
				setDigit = daysBeforeCorrection;
				setRankLimit(99, 0);
				break;
			case rank_tube_34: 
				setDigit = correctionTime; 
				setRankLimit(99, 0);
				break;
			case rank_tube_56: 
				setDigit = isForwardCorrection; 
				setRankLimit(1, 0);
				break;
		}
	} else if (tubeMode == tm_ShowHourBeep) {
		switch(rank) {
			case rank_tube_12:
				setDigit = hourBeepStart;
				setRankLimit(23, 0);
				break;
			case rank_tube_34:
				setDigit = hourBeepStop;
				setRankLimit(23, 0);
				break;
			case rank_tube_56:
				setDigit = isHourBeepActive;
				setRankLimit(1, 0);
				break;
		}
	}  else if (tubeMode == tm_ShowNightMode) {
		switch(rank) {
			case rank_tube_12:
				setDigit = hourNightModeStart;
				setRankLimit(23, 0);
				break;
			case rank_tube_34:
				setDigit = hourNightModeStop;
				setRankLimit(23, 0);
				break;
			case rank_tube_56: 
				setDigit = isNightModeActive;
				setRankLimit(1, 0);
				break;
		}
	}
}

void fillTubeValueFromIndex(uint8_t startIndex, uint8_t value) {
	startIndex = startIndex * 2 - 2;
	tubeValue[startIndex] = value < 10? 0 : value / 10;
	tubeValue[startIndex + 1] = value < 10? value : value % 10;
}

void tubeAsMode() {
	if (tubeMode == tm_ShowTime) {
		fillTubeValueFromIndex(rank_tube_12, dateTime.hour);
		fillTubeValueFromIndex(rank_tube_34, dateTime.min);
		fillTubeValueFromIndex(rank_tube_56, dateTime.sec);
	} else if (tubeMode == tm_ShowDate) {
		fillTubeValueFromIndex(rank_tube_12, dateTime.day);
		fillTubeValueFromIndex(rank_tube_34, dateTime.month);
		fillTubeValueFromIndex(rank_tube_56, dateTime.year);
	} else if (tubeMode == tm_ShowAlarm) {
		fillTubeValueFromIndex(rank_tube_12, alarmHour);
		fillTubeValueFromIndex(rank_tube_34, alarmMin);
		fillTubeValueFromIndex(rank_tube_56, isAlarmActive);
		setTube5DP();
	} else if (tubeMode == tm_ShowCorrection) {
		fillTubeValueFromIndex(rank_tube_12, daysBeforeCorrection);
		fillTubeValueFromIndex(rank_tube_34, correctionTime);
		fillTubeValueFromIndex(rank_tube_56, isForwardCorrection);
	} else if (tubeMode == tm_ShowHourBeep) {
		fillTubeValueFromIndex(rank_tube_12, hourBeepStart);
		fillTubeValueFromIndex(rank_tube_34, hourBeepStop);
		fillTubeValueFromIndex(rank_tube_56, isHourBeepActive);
	} else if (tubeMode == tm_ShowNightMode) {
		fillTubeValueFromIndex(rank_tube_12, hourNightModeStart);
		fillTubeValueFromIndex(rank_tube_34, hourNightModeStop);
		fillTubeValueFromIndex(rank_tube_56, isNightModeActive);
	}
	
	if (rank != rank_null) {
		if (tubeMode == tm_ShowTime && rank == rank_tube_56) {
			setDigit = dateTime.sec;
		}
		for (uint8_t k = rank_tube_12; k <= rank_tube_56; k++) {
			if (rank == k) {
				fillTubeValueFromIndex(rank, setDigit);
				break;
			}
		}
	}
}

void eepromWriteByte(uint8_t address, uint8_t value){
	 eeprom_write_byte((uint8_t*)(uint16_t)address, value);
}

uint8_t eepromReadByte(uint8_t address){
	return eeprom_read_byte((uint8_t*)(uint16_t)address);
}