/*
 * TeensyPOV.h
 *
 *  Created on: Apr 12, 2018
 *      Author: GFV
 */

#ifndef TEENSYPOV_H_
#define TEENSYPOV_H_

#include <Arduino.h>
#include "FastLED.h"

#if !defined(KINETISK)
#error Kinetisk required
#endif

//#define SIMULATE_RPM
//#define DEBUG_MODE

enum TextPosition {
	TOP, BOTTOM
};

struct LedArrayStruct {
	const uint32_t *array;
	const uint32_t *colors;
	uint32_t numColorBits;
	uint32_t rows;
	uint32_t columns;
	uint32_t tdcDisplaySegment;
};

struct DisplayStringSpec {
	const char *characters;
	TextPosition position;
	uint8_t topRow;
	uint8_t textColor;
	uint8_t backgroundColor;
	bool invert;
};

class TeensyPOV {
private:
	uint8_t numColorBits = 0;
	uint16_t numSegments = 0;
	const uint32_t *colorPalette = nullptr;
	const LedArrayStruct *image = nullptr;
	const DisplayStringSpec *strings = nullptr;
	uint8_t numStrings = 0;
	uint32_t displayDuration = 0, durationTimer = 0;
	uint32_t rotationPeriod = 0, rotationTimer = 0;
	int16_t rotationIncrement = 0;
	uint16_t tdcSegment = 0;
	const uint32_t *palette = nullptr;
	uint8_t idNum;

	static uint8_t numPov;
	static uint8_t currentActivePov;

public:
	TeensyPOV();
	void load(const LedArrayStruct *);
	void load(const LedArrayStruct *, const DisplayStringSpec *, uint8_t);
	void load(const DisplayStringSpec *, uint8_t);
	void activate(bool = true);
	void setDisplay(uint16_t, uint8_t, uint16_t, const uint32_t *);
	void setTiming(uint32_t, uint32_t, int16_t);
	bool update(void);
	static bool povSetup(uint8_t, CRGB *, uint8_t);
	static bool rpmGood(void);
	static void loadPalette(const uint32_t *);
	static uint32_t getLastRPMCount(void);
	static void setLed(uint16_t, uint16_t, uint32_t);
#ifdef DEBUG_MODE
	static void debugPrint(void);
#endif 		//DEBUG_MODE
};

#endif /* TEENSYPOV_H_ */
