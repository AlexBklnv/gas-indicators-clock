#include "Utilities/main.h"

void init() {
	DDRD = 0xFC;
	DDRB = 0xC7;
	DDRC = 0x0F;
	ADMUX = (1 << REFS0);
	ADCSRA |= (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
	
	i2c_init();
	_delay_us(100);
	dateTime = ds1307_getdate();
	
	bright.level = eeprom_read_byte(&eeprom_bright);
	bright.dayLevel = bright.level;
	
	hourBeep.isActive = eeprom_read_byte(&eeprom_isHourBeepActive);
	hourBeep.lastHour = dateTime.hour;
	hourBeep.manualActivation = false;
	hourBeep.beeper.initCount = 2;
	hourBeep.beeper.durationActive = 90;
	hourBeep.beeper.durationInactive = 50;
	hourBeep.beeper.waitingTime = 0;
	hourBeep.beeper.canBeep = false;
	hourBeep.beeper.isCanInit = false;
	hourBeep.beeper.isPinActive = false;
	
	alarm.isActive = eeprom_read_byte(&eeprom_isAlarmActive);
	alarm.startHour = eeprom_read_byte(&eeprom_alarmHour);
	alarm.startMin = eeprom_read_byte(&eeprom_alarmMin);
	alarm.lastDay = 0;
	alarm.isTurnedOff = false;
	alarm.beeper.initCount = 5;
	alarm.beeper.durationActive = 70;
	alarm.beeper.durationInactive = 40;
	alarm.beeper.waitingTime = 700;
	alarm.beeper.canBeep = false;
	alarm.beeper.isCanInit = false;
	alarm.beeper.isPinActive = false;
	
	ledBlinking.isCanInit = false;
	ledBlinking.durationActive = 100;
	ledBlinking.durationInactive = 50;
	ledBlinking.count = 0;
	ledBlinking.startValue = 5;
	isLedActive = eeprom_read_byte(&eeprom_ledState);

	nightMode.isActive = false;
	nightMode.thresholdInit = eeprom_read_byte(&eeprom_nightThreshold);
	nightMode.threshold =  20 * nightMode.thresholdInit;
	nightMode.isCanTryActivate = true;
	nightMode.autoStamp = 0;
	
	showDate.start = 51;
	showDate.stop = 53;
	showDate.isActive = true;
	
	switchPort(PORTB, ledPin, isLedActive);
	tube.isDoteActive[4] = alarm.isActive;
	tube.isDoteActive[5] = hourBeep.isActive;

	etching.isWorking = false;
	etching.duration = 170;
	etching.lastMin = dateTime.min;
	
	button.isLongPress = false;
	button.notInclude = false;
	button.num = 0;
	button.bounceTime = 0;
	button.longPressTime = 0;
	initMillis();
}

int main(void) {
	init();
	while (true) {
		dateTime = ds1307_getdate();
		buttonController();
		if (modeWork == mw_Clock) {
			checkNightMode();
			showDissallowedTask();
			if (alarm.isActive) {
				if (dateTime.hour == alarm.startHour && dateTime.min == alarm.startMin && dateTime.day != alarm.lastDay) {
					alarm.lastDay = dateTime.day;
					alarm.beeper.canBeep = true;
					alarm.isTurnedOff = false;
					alarm.beeper.isCanInit = true;	
				} else {
					if (dateTime.min != alarm.startMin) {
						alarm.beeper.canBeep = false;
						alarm.beeper.isCanInit = false;
					}
				}
			}
			if (hourBeep.isActive && !nightMode.isActive) {
				if (hourBeep.manualActivation || (hourBeep.lastHour != dateTime.hour && dateTime.min == 0 && alarm.startMin != 0)) 
				{
					hourBeep.manualActivation = false;
					hourBeep.lastHour = dateTime.hour;
					hourBeep.beeper.canBeep = true;
					hourBeep.beeper.isCanInit = true;
				} 
			}
			tube.isDoteActive[4] = alarm.isActive;
			tube.isDoteActive[5] = hourBeep.isActive;
			
			tubeMode = tm_ShowTime;
			if (showDate.isActive) {
				if (dateTime.sec >= showDate.start && dateTime.sec <= showDate.stop) {
					tubeMode = tm_ShowDate;
				}
			}
			
			if (dateTime.min % 10 == 0 && dateTime.sec < 3 && etching.lastMin != dateTime.min) {
				etching.lastMin = dateTime.min;
				etching.isWorking = true;
				etching.switchTime = millis();
				etching.value = 0;
			}
			if (etching.isWorking) {
				if (etching.value > 99) {
					etching.isWorking = false;
				} else { 
					if (millis() - etching.switchTime >= etching.duration) {
						etching.value+=11;
						etching.switchTime = millis();
					}
					tubeMode = tm_Etching;
				}
			}
		} else {
			if (prevSec != dateTime.sec) {
				prevSec = dateTime.sec;
				if (returnTime == 0) {
					modeWork = mw_Clock;
					for (uint8_t i = 0; i < 6; i++) {
						tube.isFlash[i] = false;
						tube.isDisabled[i] = false;
						tube.isDoteActive[i] = false;
					}
					resetButtons();
				} else {
					returnTime--;
				}
			}
			if (!editValue.isGrabbed) {
				assignEditDigit();
			}
		}
		

		if (!alarm.isTurnedOff) {
			if (alarm.beeper.canBeep && alarm.isActive) {
				ledBlinking.isCanInit = true;
			}
			if (!hourBeep.beeper.canBeep) {
				beepController(&alarm.beeper);
			}
		} else {
			alarm.beeper.canBeep = false;
		}
		if (!nightMode.isActive && !alarm.beeper.canBeep) {
			beepController(&hourBeep.beeper);
		}
		tubeAsMode();
	}
}

void translitDecoder(char tubeVal) {
	for (int i = 0; i < 4; i++) {
		tube.dc[i] = 0;
	}
	for(int i = 3; tubeVal != 0; i--) {
		tube.dc[i] = tubeVal % 2;
		tubeVal /= 2;
	}
}

void tubeSwitch() {
	switch(bright.level){
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
			if (tube.isFlash[i]) {
				if (millis() - tube.switchTime >= 150) {
					tube.isDisabled[i] =  true;
					if (millis() - tube.switchTime >= 550) {
						tube.switchTime = millis() ;
					}
				} else {
					tube.isDisabled[i] = false;
				}
			} else {
				tube.isDisabled[i] = false;
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


void checkNightMode() {
	if (getADC(7) >= nightMode.threshold) {
		if (nightMode.isCanTryActivate && millis() - nightMode.autoStamp >= 5000) {
			nightMode.isActive = true;
			bright.level = 0;
			if (ledBlinking.count == 0) {
				switchPort(PORTB,ledPin, false);
			}
			return;
		}
	} else {
		nightMode.isCanTryActivate = false;
	}
	
	
	if (!nightMode.isCanTryActivate) {
		nightMode.isCanTryActivate = true;
		nightMode.autoStamp = millis();
	}
		
	if (nightMode.isActive) {
		nightMode.isActive = false;
		bright.level = bright.dayLevel;
		if (ledBlinking.count == 0) {
			switchPort(PORTB,ledPin, isLedActive);
		}
	}
}


void resetButtons() {
	button.isLongPress = false;
	button.num = 0;
	button.notInclude = false;
	button.bounceTime = 0;
	button.longPressTime = 0;
}

int16_t getADC(uint8_t channel) {
	ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCW;
}

void buttonController() {
	if (etching.isWorking) {
		return;
	}
	button.adc = getADC(6);
	uint8_t curButtonNum = 0;

	if (button.adc < 581) {
		curButtonNum = 2;
	} else if (button.adc < 770) {
		curButtonNum = 1;
	} 
	
	if (curButtonNum > 0) {
		if (button.bounceTime == 0) {
			button.bounceTime = millis();
			button.num = curButtonNum;
		} else if (millis() - button.bounceTime > 60) {
			if (alarm.beeper.canBeep) {
				alarm.isTurnedOff = true;
				alarm.beeper.canBeep = false;
				button.notInclude = true;
			} else if (button.longPressTime == 0) {
				button.longPressTime = millis();
			} else if (millis() - button.longPressTime > 1300) {
				button.isLongPress = true;
				returnTime = 10;
				switch(button.num) {
					case 1:
						firstButtonLongPress();
						break;
					case 2:
						if (modeWork == mw_Clock && !nightMode.isActive) {
							isLedActive = !isLedActive;
							switchPort(PORTB,ledPin, isLedActive);
							eeprom_update_byte(&eeprom_ledState, isLedActive);
						}
						break;
				}
				button.longPressTime = millis();
			}
		}
	}
	
	if (curButtonNum == 0 && button.bounceTime != 0 ) {
		if (!button.isLongPress) {
			returnTime = 10;
			buttonShortPress();
		}
		resetButtons();
	}
}

void buttonShortPress() {
	if (button.notInclude) {
		button.notInclude = false;
		return;
	}
	if (modeWork == mw_Clock) {
		switch(button.num) {
			case 1:
				hourBeep.isActive = !hourBeep.isActive;
				hourBeep.manualActivation = nightMode.isActive? false: hourBeep.isActive;
				eeprom_update_byte(&eeprom_isHourBeepActive, hourBeep.isActive);
				if (nightMode.isActive) {
					ledBlinking.isCanInit = true;
					hourBeep.manualActivation = false;
				} else {
					hourBeep.manualActivation = hourBeep.isActive;
				}
				break;
			case 2:
				if (!nightMode.isActive) {
					bright.level = bright.level >= 3? 0: bright.level + 1;
					bright.dayLevel = bright.level;
					eeprom_update_byte(&eeprom_bright, bright.level);					
				} else {
					ledBlinking.isCanInit = true;
				}
				break;
		}
		return;
	} 
	
	if (modeWork == mw_SetSec) {
		if (dateTime.sec >= 30) {
			alarm.lastDay = 0;
			if (dateTime.min == 59) {
				dateTime.min = 0;
				if (dateTime.hour == 23) {
					dateTime.hour = 0;
					if (dateTime.day == getMaxMonthDay()) {
						dateTime.day = 1;
						if (dateTime.month == 12) {
							dateTime.month = 1;
							if (dateTime.year == 99) {
								dateTime.year = 00;
							} else {
								dateTime.year++;
							}
						} else {
							dateTime.month++;
						}
					} else {
						dateTime.day++;
					}
				} else {
					dateTime.hour++;
				}
				hourBeep.lastHour = dateTime.hour;
			} else {
				dateTime.min++;
			}
			etching.lastMin = dateTime.min;
		}
		dateTime.sec = 0;
		editValue.value = 0;
		prevSec = 0;
		ds1307_setdate(dateTime);
		return;
	} 
	
	switch(button.num) {
		case 1: editValue.value++; break;
		case 2: editValue.value--; break;
	}
	
	if (editValue.value > editValue.max) {
		editValue.value = editValue.min;
	} else if (editValue.value < editValue.min) {
		editValue.value = editValue.max;
	}
}

void firstButtonLongPress() {
	if (button.notInclude) {
		button.notInclude = false;
		return;
	}
	switch(modeWork) {
		case mw_SetMin:
			dateTime.min = editValue.value;
			etching.lastMin = dateTime.min;
			ds1307_setdate(dateTime);
			alarm.lastDay = 0;
			break;
		case mw_SetHour:
			dateTime.hour = editValue.value;
			hourBeep.lastHour = dateTime.hour;
			ds1307_setdate(dateTime);
			alarm.lastDay = 0;
			break;
		case mw_SetYear:
			dateTime.year = editValue.value;
			ds1307_setdate(dateTime);
			alarm.lastDay = 0;
			break;
		case mw_SetMonth:
			dateTime.month = editValue.value;
			ds1307_setdate(dateTime);
			alarm.lastDay = 0;
			break;
		case mw_SetDay:
			dateTime.day = editValue.value;
			alarm.lastDay = 0;
			ds1307_setdate(dateTime);
			break;
		case mw_SetIsAlarmActive:
			alarm.isActive = editValue.value;
			alarm.lastDay = 0;
			eeprom_update_byte(&eeprom_isAlarmActive, alarm.isActive);
			break;
		case mw_SetAlarmMin:
			alarm.startMin = editValue.value;
			alarm.lastDay = 0;
			eeprom_update_byte(&eeprom_alarmMin, alarm.startMin);
			break;
		case mw_SetAlarmHour:
			alarm.startHour = editValue.value;
			alarm.lastDay = 0;
			eeprom_update_byte(&eeprom_alarmHour, alarm.startHour);
			break;
		case mw_SetThrashhold:
			nightMode.thresholdInit = editValue.value;
			nightMode.threshold =  20 * nightMode.thresholdInit;
			eeprom_update_byte(&eeprom_nightThreshold, nightMode.thresholdInit);
			break;
	}
	
	modeWork = modeWork < mw_LastMW? modeWork + 1 : 0;
	
	for (uint8_t i = 0; i < 6; i++) {
		tube.isFlash[i] = false;
		tube.isDisabled[i] = false;
		tube.isDoteActive[i] = false;
	}
		
	editValue.isGrabbed = false;
}

void setRankLimit(int8_t upper, int8_t lower) {
	editValue.max = upper;
	editValue.min = lower;
}

uint8_t getMaxMonthDay() {
	if (dateTime.month == 2) {
		return dateTime.year % 4 == 0 ? 29 : 28;
	} else {
		if (dateTime.month < 8) {
			return dateTime.month % 2 == 0? 30 : 31;
		} else {
			return dateTime.month % 2 == 1? 30 : 31;
		}
		
	}
}

void assignEditDigit() {
	if (modeWork >= mw_SetSec && modeWork <= mw_SetHour) {
		tubeMode = tm_ShowTime;
	} else if (modeWork >= mw_SetIsAlarmActive && modeWork <= mw_SetAlarmHour) {
		tubeMode = tm_ShowAlarm;
	} else if (modeWork >= mw_SetYear && modeWork <= mw_SetDay) {
		tubeMode = tm_ShowDate;
	} else if (modeWork == mw_SetThrashhold) {
		tubeMode = tm_ShowNightModeThrashhold;
	}
	
	rank = 3 - ((modeWork + 2) % 3);
	editValue.isGrabbed = true;
	
	int8_t bufByte = rank * 2;
	for (uint8_t i = bufByte - 2; i < bufByte; i++) {
		tube.isFlash[i] = true;
	}
	
	if (tubeMode == tm_ShowTime) {
		switch(rank) {
			case rank_tube_12:
				editValue.value = dateTime.hour;
				setRankLimit(23, 0);
				break;
			case rank_tube_34:
				editValue.value = dateTime.min;
				setRankLimit(59, 0);
				break;
			case rank_tube_56:
				editValue.value = dateTime.sec;
				editValue.isGrabbed = false;
				break;
		}
		return;
	} 
	if (tubeMode == tm_ShowDate) {
		uint8_t _maxDayValue = getMaxMonthDay();
		switch(rank) {
			case rank_tube_12:
				editValue.value = dateTime.day > _maxDayValue? _maxDayValue :dateTime.day;
				setRankLimit(_maxDayValue, 0);
				break;
			case rank_tube_34:
				editValue.value = dateTime.month;
				setRankLimit(12, 1);
				break;
			case rank_tube_56:
				editValue.value = dateTime.year;
				setRankLimit(99, 00);
				break;
		}
		return;
	} 
	if (tubeMode == tm_ShowAlarm) {
		tube.isDoteActive[4] = alarm.isActive;
		switch(rank) {
			case rank_tube_12:
				editValue.value = alarm.startHour;
				setRankLimit(23, 0);
				break;
			case rank_tube_34:
				editValue.value = alarm.startMin;
				setRankLimit(59, 0);
				break;
			case rank_tube_56:
				editValue.value = alarm.isActive;
				setRankLimit(1, 0);
				break;
		}
		return;
	} 
	
	if (tubeMode == tm_ShowNightModeThrashhold) {
		editValue.value = nightMode.thresholdInit;
		setRankLimit(50, 0);
		return;
	}
}

void fillTubeValueFromIndex(uint8_t startIndex, uint8_t value) {
	startIndex = startIndex * 2 - 2;
	tube.value[startIndex] = value < 10 ? 0: value / 10;
	tube.value[startIndex + 1] = value < 10? value: value % 10;
}

void tubeAsMode() {
	uint8_t _tubeValue12 = 0;
	uint8_t _tubeValue34 = 0;
	uint8_t _tubeValue56 = 0;
	switch(tubeMode) {
		case tm_ShowTime:
			_tubeValue12 = dateTime.hour;
			_tubeValue34 = dateTime.min;
			_tubeValue56 = dateTime.sec;
			break;
		case tm_ShowDate:
			_tubeValue12 = dateTime.day;
			_tubeValue34 = dateTime.month;
			_tubeValue56 = dateTime.year;
			break;
		case tm_ShowAlarm:
			_tubeValue12 = alarm.startHour;
			_tubeValue34 = alarm.startMin;
			_tubeValue56 = alarm.isActive;
			tube.isDoteActive[4] = alarm.isActive;
			break;
		case tm_Etching:
			_tubeValue12 = etching.value;
			_tubeValue34 = etching.value;
			_tubeValue56 = etching.value;
			break;
		case tm_ShowNightModeThrashhold:
			_tubeValue12 = 0;
			_tubeValue34 = 0;
			_tubeValue56 = nightMode.thresholdInit;
			int16_t tmpThreshold = 20 * editValue.value;
			if (getADC(7) < tmpThreshold) {
				switchPort(PORTB,ledPin, true);
			} else {
				switchPort(PORTB,ledPin, false);
			}
			break;
	}
	fillTubeValueFromIndex(rank_tube_12, _tubeValue12);
	fillTubeValueFromIndex(rank_tube_34, _tubeValue34);
	fillTubeValueFromIndex(rank_tube_56, _tubeValue56);
	
	if (modeWork != mw_Clock) {
		fillTubeValueFromIndex(rank, editValue.value);
	} 
	
	for(int i = 0; i < 3; i++){
		if (!tube.isDisabled[i]) {
			translitDecoder(tube.value[i]);
			switchPort(PORTD, dpHighPin, tube.isDoteActive[i]);
			switchPort(PORTD, dcHighPin0, tube.dc[3]);
			switchPort(PORTD, dcHighPin1, tube.dc[2]);
			switchPort(PORTB, dcHighPin2, tube.dc[1]);
			switchPort(PORTB, dcHighPin3, tube.dc[0]);
		}
		if (!tube.isDisabled[i + 3]) {
			translitDecoder(tube.value[i+3]);
			switchPort(PORTC, dpLowPin, tube.isDoteActive[i+3]);
			switchPort(PORTB, dcLowPin0, tube.dc[3]);
			switchPort(PORTD, dcLowPin1, tube.dc[2]);
			switchPort(PORTD, dcLowPin2, tube.dc[1]);
			switchPort(PORTB, dcLowPin3, tube.dc[0]);
		}
		switch(i){
			case 0: switchPort(PORTC, tubePin14, 0); break;
			case 1: switchPort(PORTC, tubePin25, 0); break;
			case 2: switchPort(PORTC, tubePin36, 0); break;
		}
		tubeSwitch();
	}
}


uint8_t ds1307_dec2bcd(uint8_t val) {
	return val + 6 * (val / 10);
}

uint8_t ds1307_bcd2dec(uint8_t val) {
	return val - 6 * (val >> 4);
}

void ds1307_setdate(DateTime dateTime) {
	i2c_start_wait(DS1307_ADDR | I2C_WRITE);
	i2c_write(0x00);
	i2c_write(ds1307_dec2bcd(dateTime.sec));
	i2c_write(ds1307_dec2bcd(dateTime.min));
	i2c_write(ds1307_dec2bcd(dateTime.hour));
	i2c_write(ds1307_dec2bcd(1));
	i2c_write(ds1307_dec2bcd(dateTime.day));
	i2c_write(ds1307_dec2bcd(dateTime.month));
	i2c_write(ds1307_dec2bcd(dateTime.year));
	i2c_write(0x00);
	i2c_stop();
}

DateTime ds1307_getdate() {
	i2c_start_wait(DS1307_ADDR | I2C_WRITE);
	i2c_write(0x00);
	i2c_stop();
	DateTime dateTime;
	i2c_rep_start(DS1307_ADDR | I2C_READ);
	dateTime.sec = ds1307_bcd2dec(i2c_readAck() & 0x7F);
	dateTime.min = ds1307_bcd2dec(i2c_readAck());
	dateTime.hour = ds1307_bcd2dec(i2c_readAck());
	i2c_readAck();
	dateTime.day = ds1307_bcd2dec(i2c_readAck());
	dateTime.month = ds1307_bcd2dec(i2c_readAck());
	dateTime.year = ds1307_bcd2dec(i2c_readNak());
	i2c_stop();
	return dateTime;
}

void beepController(Beeper* beeper) {
	if (!beeper->canBeep) {
		switchPort(PORTD, beepPin, 0);
		beeper->isPinActive = false;
		return;
	}
	if (beeper->isCanInit) {
		switchPort(PORTD, beepPin, 1);
		beeper->isPinActive = true;
		beeper->beepTime = millis();
		beeper->count = beeper->initCount - 1;
		beeper->isCanInit = false;
	} else {
		if (beeper->isPinActive) {
			if (millis() - beeper->beepTime >= beeper->durationActive) {
				switchPort(PORTD, beepPin, 0);
				beeper->isPinActive = false;
				beeper->beepTime = millis();
			}
		} else {
			if (beeper->count > 0) {
				if (millis() - beeper->beepTime >= beeper->durationInactive) {
					switchPort(PORTD, beepPin, 1);
					beeper->isPinActive = true;
					beeper->beepTime = millis();
					beeper->count--;
				}
			} else if (beeper->waitingTime != 0 && millis() - beeper->beepTime >= beeper->waitingTime){
				beeper->isCanInit = true;
			}
		}
	}
}


void showDissallowedTask() {
	if (!nightMode.isActive && !(alarm.beeper.canBeep && alarm.isActive)) {
		switchPort(PORTB,ledPin, isLedActive);
		return;
	} 
	
	if (ledBlinking.isCanInit && ledBlinking.count == 0) {
		switchPort(PORTB,ledPin, true);
		ledBlinking.ledTime = millis();
		ledBlinking.count = ledBlinking.startValue;
		ledBlinking.isCanInit = false;
	} else {
		if (isPortHigh(PINB,ledPin) && ledBlinking.count > 0) {
			if (millis() - ledBlinking.ledTime >= ledBlinking.durationActive) {
				switchPort(PORTB,ledPin, false);
				ledBlinking.count--;
				ledBlinking.ledTime = millis();
			}
		} else {
			if (ledBlinking.count > 0) {
				if (millis() - ledBlinking.ledTime >= ledBlinking.durationInactive) {
					switchPort(PORTB,ledPin, true);
					ledBlinking.ledTime = millis();
				}
			}
		}
	}
	
}