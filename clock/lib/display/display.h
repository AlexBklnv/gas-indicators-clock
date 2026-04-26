#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define TUBE_COUNT           6
#define TUBE_PAIR_COUNT      3
#define BRIGHTNESS_MAX       3
#define BRIGHTNESS_MIN       0
#define ETCHING_DURATION_MS  170
#define ETCHING_INTERVAL_MIN 10

// Tube ranks (pair indices)
#define RANK_TUBE_12    1
#define RANK_TUBE_34    2
#define RANK_TUBE_56    3

struct Brightness {
	uint8_t level;
	uint8_t dayLevel;
};

struct Tube {
	bool isFlash[TUBE_COUNT];
	bool isDisabled[TUBE_COUNT];
	bool isDoteActive[TUBE_COUNT];
	uint8_t decoderBits[4];
	uint8_t value[TUBE_COUNT];
	uint32_t switchTime;
};

struct Etching {
	bool isWorking;
	uint8_t lastMinute;
	uint8_t value;
	uint16_t duration;
	uint32_t switchTime;
};

struct ShowDate {
	uint8_t startSecond;
	uint8_t stopSecond;
	bool isActive;
};

enum TubeMode : int8_t {
	TM_ETCHING,
	TM_SHOW_TIME,
	TM_SHOW_ALARM1,
	TM_SHOW_ALARM2,
	TM_SHOW_DATE,
	TM_SHOW_DATE_SETTINGS,
	TM_SHOW_NIGHT_MODE,
	TM_SHOW_NIGHT_THRESHOLD,
	TM_SHOW_CORRECTION
};

extern Brightness brightness;
extern Tube tube;
extern Etching etching;
extern ShowDate showDate;
extern TubeMode tubeMode;

void display_init(void);
void display_render(uint8_t menuMode, uint8_t rank, int16_t editValue);
void display_fillPair(uint8_t pairIndex, uint8_t value);
void display_resetFlags(void);

void etching_check(uint8_t currentMinute, uint8_t currentSecond);
bool etching_update(void);

#endif // DISPLAY_H
