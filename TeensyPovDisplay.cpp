/*
 * TeensyPovDisplay.cpp
 *
 *  Created on: Apr 12, 2018
 *      Author: GFV
 */

#include "TeensyPovDisplay.h"

uint8_t TeensyPovDisplay::numPov = 0;
uint8_t TeensyPovDisplay::currentActivePov = 0;

TeensyPovDisplay::TeensyPovDisplay() {
	/*
	 * Constructor
	 */
	idNum = ++numPov;
}

void TeensyPovDisplay::load(const LedArrayStruct *pattern) {
	/*
	 * Load a TeensyPOV object with a bit pattern image
	 * Parameters:
	 * 		LedArrayStruct *pattern -- Pointer to the image structure.
	 * 			The struct pointed to must be static or global.
	 *
	 * Returns:
	 * 		N/A
	 */
	image = pattern;
	logNumSegments = pattern->logNumSegments;
	numColorBits = pattern->numColorBits;
	colorPalette = pattern->colors;
	tdcSegment = pattern->tdcDisplaySegment;
	numStrings = 0;
	strings = nullptr;

	displayDuration = 0;
	durationTimer = 0;
	rotationPeriod = 0;
	rotationTimer = 0;
	rotationIncrement = 0;
	activationCallback = nullptr;
	updateCallback = nullptr;
	expireCallback = nullptr;
}

void TeensyPovDisplay::load(const LedArrayStruct *pattern,
		const DisplayStringSpec *strArray, uint8_t n) {
	/*
	 * Load a TeensyPOV object with a bit pattern image and text strings
	 * Parameters:
	 * 		LedArrayStruct *pattern -- Pointer to the image structure
	 * 			The struct pointed to must be static or global.
	 *
	 * 		DisplayStringSpec *strArray -- Pointer to an array of DisplayStringSpec structures
	 * 			The array pointed to must be static or global.
	 *
	 * 		uint8_t n -- Number of DisplayStringSpec structures in the array
	 *
	 * Returns:
	 * 		N/A
	 */
	image = pattern;
	logNumSegments = pattern->logNumSegments;
	numColorBits = pattern->numColorBits;
	colorPalette = pattern->colors;
	tdcSegment = pattern->tdcDisplaySegment;
	strings = strArray;
	numStrings = n;

	displayDuration = 0;
	durationTimer = 0;
	rotationPeriod = 0;
	rotationTimer = 0;
	rotationIncrement = 0;
	activationCallback = nullptr;
	updateCallback = nullptr;
	expireCallback = nullptr;
}

void TeensyPovDisplay::load(const DisplayStringSpec *strArray, uint8_t n) {
	/*
	 * Load a TeensyPOV object with text strings
	 * Parameters:
	 * 		DisplayStringSpec *strArray -- Pointer to an array of DisplayStringSpec structures
	 * 			The array pointed to must be static or global.
	 *
	 * 		uint8_t n -- Number of DisplayStringSpec structures in the array
	 *
	 * Returns:
	 * 		N/A
	 */
	strings = strArray;
	numStrings = n;
	//tdcSegment = 0;
	image = nullptr;

	displayDuration = 0;
	durationTimer = 0;
	rotationPeriod = 0;
	rotationTimer = 0;
	rotationIncrement = 0;
	activationCallback = nullptr;
	updateCallback = nullptr;
	expireCallback = nullptr;
}

void TeensyPovDisplay::load() {
	/*
	 * Load an empty TeensyPOV object. Would be used if no bitmap image or text is desired.
	 * Pixels would be set by the main program using the setLed() method.
	 * Parameters:
	 * 	N/A
	 *
	 * Returns:
	 *	N/A
	 *
	 */
	strings = nullptr;
	numStrings = 0;
	//tdcSegment = 0;
	image = nullptr;
	displayDuration = 0;
	durationTimer = 0;
	rotationPeriod = 0;
	rotationTimer = 0;
	rotationIncrement = 0;
	activationCallback = nullptr;
	updateCallback = nullptr;
	expireCallback = nullptr;
}

void TeensyPovDisplay::setDisplay(uint8_t logSeg, uint8_t cBits, uint16_t tdc,
		const uint32_t *colors) {
	/*
	 * Load the POV parameters for a TeensyPOV object that does not contain a bitmap image (i.e. a LedArrayStruct).
	 * Call this function after load() and before activate().
	 *  Parameters:
	 *  	uint16_t seg -- Number of POV segments to use (128 and 256 both work well)
	 *
	 *  	uint8_t cBits -- Number of color bits (1-6)
	 *
	 *  	uint16_t tdc -- Segment number to display when rotating blade hits Top Dead Center
	 *
	 *  	const uint32_t *colors -- Pointer to array of RGB colors. Array must have at
	 *  		least 2 ^ cBits elements. The array pointed to must be static or global.
	 *
	 * Returns:
	 * 	N/A
	 */
	logNumSegments = logSeg;
	numColorBits = cBits;
	tdcSegment = tdc;
	colorPalette = colors;
}

void TeensyPovDisplay::setTiming(uint32_t duration, uint32_t rotation,
		int16_t tdcDelta) {
	/*
	 * Set timing for dynamic TeensyPOV objects. Optional, all values are set to their default values by the load() method.
	 *
	 * Parameters:
	 * 	uint32_t duration -- Time (in milliseconds) before the object expires. Use zero (default) if object is permanent.
	 *
	 * 	uint32_t rotation -- Time (in milliseconds) between changes in the Top Dead Center segment, causes display to rotate.
	 * 		Use zero (default)  for no rotation
	 *
	 * 	int16_t tdcDelta -- Adjustment to Top Dead Center segment every 'rotation' milliseconds. Use negative value for clockwise rotation and
	 * 		positive value for anti-clockwise. Has no effect if 'rotation' is zero.
	 *
	 * Returns:
	 * 	N/A
	 *
	 */
	displayDuration = duration;
	rotationPeriod = rotation;
	rotationIncrement = tdcDelta;
}

void TeensyPovDisplay::activate() {
	/*
	 * Display a TeensyPOV object on the POV LEDs.
	 * Resets the duration and rotation timers
	 *  Also used to refresh a currently active display after changing
	 * text, bit image, palette, etc.
	 * Parameters:
	 * 		N/A
	 *
	 * Returns:
	 * 		N/A
	 */
	loadPovStructures(true);
}

void TeensyPovDisplay::refresh() {
	/*
	 * Refresh the TeensyPOV object on the POV LEDs after changing text, bit image, palette, etc.
	 * Does not resets the duration and rotation timers
	 * Parameters:
	 * 		N/A
	 *
	 * Returns:
	 * 		N/A
	 */
	if (currentActivePov != idNum) {
		return;
	}
	loadPovStructures(false);
}


void TeensyPovDisplay::setActivationCallback(void (*ptr)(TeensyPovDisplay *)) {
	/* Set optional callback function to be called when a TeensyPOV object is activated.
	 *  Parameters:
	 *  	void (*ptr)(TeensyPOV *) - pointer to the function to be called. The function must take as an argument a pointer to the
	 *  		TeensyPOV object that was activated and return void
	 */
	activationCallback = ptr;
}

void TeensyPovDisplay::setUpdateCallback(void (*ptr)(TeensyPovDisplay *)) {
	/* Set optional callback function to be called when a TeensyPOV object is updated.
	 *  Parameters:
	 *  	void (*ptr)(TeensyPOV *) - pointer to the function to be called. The function must take as an argument a pointer to the
	 *  		TeensyPOV object that was updated and return void
	 */
	updateCallback = ptr;
}

void TeensyPovDisplay::setExpireCallback(void (*ptr)(TeensyPovDisplay *)) {
	/* Set optional callback function to be called when a TeensyPOV object expires.
	 *  Parameters:
	 *  	void (*ptr)(TeensyPOV *) - pointer to the function to be called. The function must take as an argument a pointer to the
	 *  		TeensyPOV object that expired and return void
	 */
	expireCallback = ptr;
}


bool TeensyPovDisplay::update() {
	/*
	 * Determines if duration of TeensyPOV object has expired and performs specified rotation.
	 *	Parameters:
	 *		N/A
	 *
	 *	Returns:
	 *		true -- if the object has expired.
	 *		false -- otherwise.
	 */
	uint32_t currentMillis;

	currentMillis = millis();
	if (idNum != currentActivePov) {
		return true;
	}

	if (expired) {
		return true;
	}

	if (rotationPeriod > 0) {
		if (currentMillis - rotationTimer >= rotationPeriod) {
			rotationTimer += rotationPeriod;
			TeensyPOV::updateTdcDisplaySegment =
					(TeensyPOV::currentTdcDisplaySegment + rotationIncrement)
							& TeensyPOV::currentSegmentMask;
		}
	}

	if (displayDuration > 0) {
		if (currentMillis - durationTimer >= displayDuration) {
			if (expireCallback) {
				expireCallback(this);
			}
			return true;
		}
	}

	if (updateCallback) {
		updateCallback(this);
	}

	return false;
}

void TeensyPovDisplay::loadPovStructures(bool startTiming) {
	uint8_t index;
	const DisplayStringSpec *strPtr;
	if (currentActivePov != idNum) {
		TeensyPOV::setParameters(logNumSegments, numColorBits, tdcSegment);
	}

	TeensyPOV::loadColors(colorPalette);

	if (image) {
		TeensyPOV::loadPattern(image);
	}

	if (strings) {
		for (index = 0; index < numStrings; index++) {
			strPtr = strings + index;
			TeensyPOV::loadString(strPtr->characters, strPtr->position,
					strPtr->topRow, strPtr->textColor, strPtr->backgroundColor,
					strPtr->invert);
		}
	}
	currentActivePov = idNum;

	if (startTiming) {
		expired = false;
		durationTimer = millis();
		rotationTimer = durationTimer;
		if (activationCallback) {
			activationCallback(this);
		}
	}
}

