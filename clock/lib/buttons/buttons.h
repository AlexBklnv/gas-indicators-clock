#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

// Button ADC thresholds (override in hardware.h if needed)
#ifndef BTN_THRESHOLD_BUTTON2
#define BTN_THRESHOLD_BUTTON2   581
#endif
#ifndef BTN_THRESHOLD_BUTTON1
#define BTN_THRESHOLD_BUTTON1   770
#endif

// Timing
#define BUTTON_DEBOUNCE_MS      60
#define BUTTON_LONG_PRESS_MS    1300

enum ButtonEvent : uint8_t {
	BTN_NONE = 0,
	BTN_DEBOUNCED,    // any button just passed debounce (fires once)
	BTN_SHORT_1,
	BTN_SHORT_2,
	BTN_LONG_1,
	BTN_LONG_2
};

void buttons_init(void);
ButtonEvent buttons_update(bool blockInput);
void buttons_consume(void);

#endif // BUTTONS_H
