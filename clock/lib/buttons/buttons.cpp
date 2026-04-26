#include "buttons.h"
#include "hardware.h"
#include "millis.h"
#include "adc.h"

static struct {
	bool isLongPress;
	bool consumed;
	bool debounceEventFired;
	uint8_t activeButton;
	uint32_t bounceTime;
	uint32_t longPressTime;
} buttonState;

void buttons_init(void) {
	buttonState.isLongPress = false;
	buttonState.consumed = false;
	buttonState.debounceEventFired = false;
	buttonState.activeButton = 0;
	buttonState.bounceTime = 0;
	buttonState.longPressTime = 0;
}

void buttons_consume(void) {
	buttonState.consumed = true;
}

ButtonEvent buttons_update(bool blockInput) {
	if (blockInput) {
		return BTN_NONE;
	}

	int16_t adcValue = adc_read(ADC_CHANNEL_BUTTONS);
	uint8_t currentButton = 0;

	if (adcValue < BTN_THRESHOLD_BUTTON2) {
		currentButton = 2;
	} else if (adcValue < BTN_THRESHOLD_BUTTON1) {
		currentButton = 1;
	}

	ButtonEvent event = BTN_NONE;

	if (currentButton > 0) {
		if (buttonState.bounceTime == 0) {
			buttonState.bounceTime = millis();
			buttonState.activeButton = currentButton;
			buttonState.debounceEventFired = false;
		} else if (millis() - buttonState.bounceTime > BUTTON_DEBOUNCE_MS) {
			if (!buttonState.debounceEventFired) {
				buttonState.debounceEventFired = true;
				event = BTN_DEBOUNCED;
			} else if (!buttonState.consumed) {
				if (buttonState.longPressTime == 0) {
					buttonState.longPressTime = millis();
				} else if (millis() - buttonState.longPressTime > BUTTON_LONG_PRESS_MS) {
					buttonState.isLongPress = true;
					event = (buttonState.activeButton == 1) ? BTN_LONG_1 : BTN_LONG_2;
					buttonState.longPressTime = millis(); // auto-repeat
				}
			}
		}
	}

	// Button released
	if (currentButton == 0 && buttonState.bounceTime != 0) {
		if (!buttonState.isLongPress && !buttonState.consumed) {
			event = (buttonState.activeButton == 1) ? BTN_SHORT_1 : BTN_SHORT_2;
		}
		// Reset state
		buttonState.isLongPress = false;
		buttonState.consumed = false;
		buttonState.debounceEventFired = false;
		buttonState.activeButton = 0;
		buttonState.bounceTime = 0;
		buttonState.longPressTime = 0;
	}

	return event;
}
