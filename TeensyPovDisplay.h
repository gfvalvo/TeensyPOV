/*
 * TeensyPovDisplay.h
 *
 *  Created on: Apr 12, 2018
 *      Author: GFV
 */

#ifndef TEENSYPOVDISPLAY_H_
#define TEENSYPOVDISPLAY_H_

#include <Arduino.h>
#define FASTLED_INTERNAL
#include "FastLED.h"
#include "TeensyPOV.h"

#if !defined(KINETISK)
#error Kinetisk required
#endif

class TeensyPovDisplay {
private:
	uint8_t numColorBits = 0;
	uint8_t logNumSegments = 1;
	const uint32_t *colorPalette = nullptr;
	const LedArrayStruct *image = nullptr;
	const DisplayStringSpec *strings = nullptr;
	uint8_t numStrings = 0;
	uint32_t displayDuration = 0, durationTimer = 0;
	uint32_t rotationPeriod = 0, rotationTimer = 0;
	int16_t rotationIncrement = 0;
	uint16_t tdcSegment = 0;
	uint8_t idNum;
	bool expired = false;
	void loadPovStructures(bool);
	void (*activationCallback)(TeensyPovDisplay *) = nullptr;
	void (*updateCallback)(TeensyPovDisplay *) = nullptr;
	void (*expireCallback)(TeensyPovDisplay *) = nullptr;

	static uint8_t numPov;
	static uint8_t currentActivePov;

public:
	TeensyPovDisplay();
	void load(const LedArrayStruct *);
	void load(const LedArrayStruct *, const DisplayStringSpec *, uint8_t);
	void load(const DisplayStringSpec *, uint8_t);
	void load();
	void activate();
	void refresh();
	void setDisplay(uint8_t, uint8_t, uint16_t, const uint32_t *);
	void setTiming(uint32_t, uint32_t, int16_t);
	void setActivationCallback(void (*)(TeensyPovDisplay *));
	void setUpdateCallback(void (*)(TeensyPovDisplay *));
	void setExpireCallback(void (*)(TeensyPovDisplay *));
	bool update(void);
};

#endif /* TEENSYPOVDISPLAY_H_ */
