/*
 * TeensyPOV.cpp
 *
 *  Created on: Apr 12, 2018
 *      Author: GFV
 */
#include "TeensyPOV.h"
#include "textCharacters.h"

const uint8_t LOG_2_SEGMENTS = 1;
const uint8_t LOG_4_SEGMENTS = 2;
const uint8_t LOG_8_SEGMENTS = 3;
const uint8_t LOG_16_SEGMENTS = 4;
const uint8_t LOG_32_SEGMENTS = 5;
const uint8_t LOG_64_SEGMENTS = 6;
const uint8_t LOG_128_SEGMENTS = 7;
const uint8_t LOG_256_SEGMENTS = 8;
const uint8_t LOG_512_SEGMENTS = 9;

static void dummy_funct(void);
static void rpmTimerIsr(void);
static void segmentTimerIsr(void);
static void tdcIsrInit(void);
static void tdcIsrActive(void);
static void mainTdcISR(void);
static void ledOffISR(void);
static void updateLeds(void);
static void allLedsOff(void);
static void loadPattern(const LedArrayStruct *);
static void loadColors(const uint32_t *);
static void setParameters(uint8_t, uint8_t, uint16_t);
static void loadString(const char *, TextPosition, uint8_t, uint8_t, uint8_t,
		bool);
static void setPixel(uint16_t, uint16_t, uint32_t);
static uint16_t virtualToPhysicalSegment(int16_t);

#ifndef SIMULATE_RPM
static const uint32_t maxRevolutionPeriod = 100000UL; // Only run LEDs when > 10 revs / sec (600 RPM)
#else
static const uint32_t maxRevolutionPeriod = 30000000UL;
#endif  //SIMULATE_RPM

static const uint32_t rpmCycles = (F_BUS / 1000000UL) * maxRevolutionPeriod - 1;

static const uint32_t maxNumLeds = 50;
static const uint32_t maxNumColorBits = 6;
static const uint32_t maxNumSegments = 1 << LOG_512_SEGMENTS;
static const uint32_t bitsPerSegment = maxNumLeds * maxNumColorBits;
static const uint32_t bitsPerWord =
		((maxNumColorBits == 3) || (maxNumColorBits == 5)
				|| (maxNumColorBits == 6)) ? 30 : 32;
static const uint32_t remainderBits = bitsPerSegment % bitsPerWord;
static const uint32_t extraWord = (remainderBits != 0) ? 1 : 0;
static const uint32_t maxColumns = (bitsPerSegment / bitsPerWord) + extraWord;
static const uint8_t maxTextChars = maxNumSegments / (2 * 7);
static const uint8_t minGoodRpmCount = 2;

static uint8_t pixelsPerWord;
static uint32_t numLeds;
static CRGB *leds;

volatile static uint8_t currentLogNumSegments = 1;
volatile static uint32_t segmentArray[maxNumSegments][maxColumns];
volatile static uint32_t colorArray[1 << maxNumColorBits];
volatile static uint32_t currentNumColorBits = 0;
volatile static uint32_t currentNumSegments = 1 << currentLogNumSegments;
volatile static uint32_t currentSegmentMask = currentNumSegments - 1;
volatile static uint32_t bitCountLoad, currentColorMask;
volatile static uint32_t goodRpmCount;
volatile static uint32_t currentDisplaySegment;
volatile static uint32_t currentTdcDisplaySegment = 0;

volatile static uint32_t lastRpmTimerReading;
volatile static uint8_t hallPin;

static KINETISK_PIT_CHANNEL_t *rpmTimer, *segmentTimer, *ledOnTimer;

static void (*funct_table[4])() = {dummy_funct, dummy_funct, dummy_funct, dummy_funct};
static void (*tdcInteruptVector)() = dummy_funct;

#ifdef SIMULATE_RPM
static const uint32_t tdcSimulatorCycles = (F_BUS / 1000000UL) * 52174UL - 1;
static KINETISK_PIT_CHANNEL_t *tdcSimulator;
#endif  // SIMULATE_RPM

#ifdef DEBUG_MODE
volatile static bool segmentTimerIsrFire = false;
volatile static bool rpmTimerIsrFire = false;
volatile static bool missedSegment = false;
volatile static uint8_t tdcIsrFire = 0;
volatile static uint32_t lastSegment;
#endif  // DEBUG_MODE

uint8_t TeensyPOV::numPov = 0;
uint8_t TeensyPOV::currentActivePov = 0;

TeensyPOV::TeensyPOV() {
	/*
	 * Constructor
	 */
	idNum = ++numPov;
}

void TeensyPOV::load(const LedArrayStruct *pattern) {
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

void TeensyPOV::load(const LedArrayStruct *pattern,
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

void TeensyPOV::load(const DisplayStringSpec *strArray, uint8_t n) {
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

void TeensyPOV::load() {
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

void TeensyPOV::setDisplay(uint8_t logSeg, uint8_t cBits, uint16_t tdc,
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

void TeensyPOV::setTiming(uint32_t duration, uint32_t rotation,
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

void TeensyPOV::activate() {
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

void TeensyPOV::refresh() {
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

void TeensyPOV::loadPovStructures(bool startTiming) {
	uint8_t index;
	const DisplayStringSpec *strPtr;
	if (currentActivePov != idNum) {
		setParameters(logNumSegments, numColorBits, tdcSegment);
	}

	loadColors(colorPalette);

	if (image) {
		loadPattern(image);
	}

	if (strings) {
		for (index = 0; index < numStrings; index++) {
			strPtr = strings + index;
			loadString(strPtr->characters, strPtr->position, strPtr->topRow,
					strPtr->textColor, strPtr->backgroundColor, strPtr->invert);
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

void TeensyPOV::setActivationCallback(void (*ptr)(TeensyPOV *)) {
	/* Set optional callback function to be called when a TeensyPOV object is activated.
	 *  Parameters:
	 *  	void (*ptr)(TeensyPOV *) - pointer to the function to be called. The function must take as an argument a pointer to the
	 *  		TeensyPOV object that was activated and return void
	 */
	activationCallback = ptr;
}

void TeensyPOV::setUpdateCallback(void (*ptr)(TeensyPOV *)) {
	/* Set optional callback function to be called when a TeensyPOV object is updated.
	 *  Parameters:
	 *  	void (*ptr)(TeensyPOV *) - pointer to the function to be called. The function must take as an argument a pointer to the
	 *  		TeensyPOV object that was updated and return void
	 */
	updateCallback = ptr;
}

void TeensyPOV::setExpireCallback(void (*ptr)(TeensyPOV *)) {
	/* Set optional callback function to be called when a TeensyPOV object expires.
	 *  Parameters:
	 *  	void (*ptr)(TeensyPOV *) - pointer to the function to be called. The function must take as an argument a pointer to the
	 *  		TeensyPOV object that expired and return void
	 */
	expireCallback = ptr;
}

void TeensyPOV::setLed(uint16_t segment, uint16_t led, uint32_t value) {
	/*
	 * Set a LED directly on the display.
	 * Parameters:
	 * 	uint16_t segment -- Rotational segment of the LED, counted clockwise from Top Dead Center. Value between 0 and (Number of Segments)-1
	 *
	 * 	uint16_t led -- Radial position of the LED, 0 = innermost.
	 *
	 * 	uint32_t value -- Color of the LED expressed as index into current Palette.
	 *
	 * Returns:
	 * 		N/A
	 */
	setPixel(segment, led, value);
}

bool TeensyPOV::rpmGood() {
	/*
	 * Report if POV blade is spinning at sufficient rotational speed
	 * Parameters:
	 *			None
	 *  Returns:
	 * 			true - speed is good
	 * 			false - otherwise
	 */
	return (tdcInteruptVector == tdcIsrActive);
}

uint32_t TeensyPOV::getLastRotationCount() {
	/*
	 * Get duration of last POV blade rotation in units of PIT ticks.
	 * On a Teensy 3.2 there are 48,000,000 PIT ticks per second.
	 * This value can be used to compute the blade's rotational speed.
	 *	Parameters:
	 *			N/A
	 *	Returns:
	 * 			Count of PIT ticks (uint32_t)
	 */
	return rpmCycles - lastRpmTimerReading;
}

bool TeensyPOV::update() {
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
	int16_t segment;

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
			segment = currentTdcDisplaySegment + rotationIncrement;
			currentTdcDisplaySegment = virtualToPhysicalSegment(segment);
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

uint16_t TeensyPOV::getNumSegments() {
	return currentNumSegments;
}

bool TeensyPOV::povSetup(uint8_t hPin, CRGB *ledPtr, uint8_t num) {
	/*
	 * Initialize the POV setup.
	 * Parameters:
	 * 	uint8_t hPin -- Pin that the (active low) Hall Effect sensor is connected to.
	 *
	 * 	CRGB *ledPtr -- Pointer to the array of FastLED CRGB objects. Before calling this method, the CRBG objects should be registered
	 * 		with FastLED using the FastLED.addLeds() method.
	 *
	 * 	uint8_t num -- Number of LEDs
	 *
	 * 	Returns:
	 * 		true if num < maxNumLeds
	 */
	const uint8_t rpmTimerIndex = 0;
	const uint8_t segmentTimerIndex = 1;
	const uint8_t ledOnTimerIndex = 2;

	const uint8_t rpmTimerInterruptPriority = 128;
	const uint8_t segmentTimerInterruptPriority = 128;
	const uint8_t ledOnTimerTimerInterruptPriority = 128;

	const uint32_t ledOnPeriod = 20UL; // Leave LEDs on Time
	const uint32_t ledOnCycles = (F_BUS / 1000000UL) * ledOnPeriod - 1;

	if (num > maxNumLeds) {
		return false;
	}
	hallPin = hPin;
	leds = ledPtr;
	numLeds = num;
	allLedsOff(); // Initialize LEDs and set all off

	// Enable Periodic Interrupt Timers (PIT) - from PJRC Teensy IntervalTimer.cpp code
	SIM_SCGC6 |= SIM_SCGC6_PIT;
	// solves timing problem on Teensy 3.5
	__asm__ volatile("nop");
	PIT_MCR = 1;

	// Assign PITs
	rpmTimer = KINETISK_PIT_CHANNELS + rpmTimerIndex;
	segmentTimer = KINETISK_PIT_CHANNELS + segmentTimerIndex;
	ledOnTimer = KINETISK_PIT_CHANNELS + ledOnTimerIndex;

	// Configure RPM Watchdog PIT
	rpmTimer->TCTRL = 0;							// Disable PIT
	rpmTimer->TFLG = 1;								// Clear interrupt flag
	rpmTimer->LDVAL = rpmCycles;					// Set count down value
	funct_table[rpmTimerIndex] = rpmTimerIsr;		// Set ISR
	NVIC_SET_PRIORITY(IRQ_PIT_CH0 + rpmTimerIndex, rpmTimerInterruptPriority);// Set interrupt priority
	NVIC_ENABLE_IRQ(IRQ_PIT_CH0 + rpmTimerIndex);			// Enable interrupt

	// Configure segmentTimer PIT to set LEDs at each rotation position
	segmentTimer->TCTRL = 0;		// Disable PIT will be enabled in tdcISR()
	segmentTimer->TFLG = 1;			// Clear interrupt flag
	funct_table[segmentTimerIndex] = segmentTimerIsr;		// Set ISR
	NVIC_SET_PRIORITY(IRQ_PIT_CH0 + segmentTimerIndex,
			segmentTimerInterruptPriority);	// Set interrupt priority
	NVIC_ENABLE_IRQ(IRQ_PIT_CH0 + segmentTimerIndex);		// Enable interrupt

	// Configure LED Off PIT to turn off leds ~20us after being turned on
	ledOnTimer->TCTRL = 0;		// Disable PIT will be enabled in tdcISR()
	ledOnTimer->TFLG = 1;			// Clear interrupt flag
	ledOnTimer->LDVAL = ledOnCycles;					// Set count down value
	funct_table[ledOnTimerIndex] = ledOffISR;		// Set ISR
	NVIC_SET_PRIORITY(IRQ_PIT_CH0 + ledOnTimerIndex,
			ledOnTimerTimerInterruptPriority);	// Set interrupt priority
	NVIC_ENABLE_IRQ(IRQ_PIT_CH0 + ledOnTimerIndex);	// Enable interrupt

#ifdef SIMULATE_RPM
	// Set up timer interrupt to simulate Hall sensor (Top Dead Center)
	const uint8_t tdcSimulatorTimerIndex = 3;
	const uint8_t tdcSimulatorInterruptPriority = 128;

	tdcSimulator = KINETISK_PIT_CHANNELS + tdcSimulatorTimerIndex;
	tdcSimulator->TCTRL = 0;// Disable PIT
	tdcSimulator->TFLG = 1;// Clear interrupt flag
	tdcSimulator->LDVAL = tdcSimulatorCycles;// Set count down value
	funct_table[tdcSimulatorTimerIndex] = mainTdcISR;// Set ISR
	NVIC_SET_PRIORITY(IRQ_PIT_CH0 + tdcSimulatorTimerIndex,
			tdcSimulatorInterruptPriority);// Set interrupt priority
	NVIC_ENABLE_IRQ(IRQ_PIT_CH0 + tdcSimulatorTimerIndex);// Enable interrupt
	tdcSimulator->TCTRL = 3;

#else
	// Set up interrupt for Hall sensor (Top Dead Center)
	pinMode(hallPin, INPUT_PULLUP);
	attachInterrupt(hallPin, mainTdcISR, FALLING);
#endif  // SIMULATE_RPM

	setParameters(1, 1, 0);
	return true;
}

static void loadPattern(const LedArrayStruct *patternStruct) {
	uint32_t row, column;

	for (row = 0; row < currentNumSegments; row++) {
		for (column = 0; column < patternStruct->columns; column++) {
			segmentArray[row][column] = *(patternStruct->array
					+ row * patternStruct->columns + column);
		}
	}
}

static void loadString(const char *string, TextPosition pos, uint8_t topLed,
		uint8_t color, uint8_t background, bool invert) {
	char charBuffer[maxTextChars + 1], *bufferPosition;
	uint8_t len, fontMask, ledBit;
	//uint8_t padPosition;
	uint8_t charMatrix[5];
	int8_t bufferPositionDelta;
	uint16_t currentMaxChars, stopLed, startLed, physicalSegment;
	uint16_t charCounter, pixelCounter, matrixCounter;
	int16_t virtualSegment;

	if (topLed >= numLeds) {
		return;
	}

	if (topLed < 6) {
		return;
	}

	stopLed = topLed;
	startLed = stopLed - 6;

	//memset(charBuffer, ' ', maxTextChars);
	//charBuffer[maxTextChars] = '\0';
	memset(charBuffer, '\0', maxTextChars);

	currentMaxChars = currentNumSegments / (2 * 7);
	len = strlen(string);
	if (len > currentMaxChars) {
		len = currentMaxChars;
	}
	//padPosition = (currentMaxChars - len) / 2;
	//strncpy(charBuffer + padPosition, string, len);
	strncpy(charBuffer, string, len);

	switch (pos) {
	case TOP:
		virtualSegment = 3 * currentNumSegments / 4 + 1;
		virtualSegment += (currentNumSegments / 2 - len * 7) / 2;
		bufferPosition = charBuffer;
		bufferPositionDelta = 1;
		break;

	case BOTTOM:
		virtualSegment = currentNumSegments / 4 + 1;
		virtualSegment += (currentNumSegments / 2 - len * 7) / 2;
		if (invert) {
			bufferPosition = charBuffer + len - 1;
			bufferPositionDelta = -1;
		} else {
			bufferPosition = charBuffer;
			bufferPositionDelta = 1;
		}
		break;

	default:
		return;
	}

	physicalSegment = virtualToPhysicalSegment(virtualSegment);
	//for (charCounter = 0; charCounter < currentMaxChars; charCounter++) {
	for (charCounter = 0; charCounter < len; charCounter++) {
		getMatrix(*bufferPosition, charMatrix, invert);

		for (pixelCounter = startLed; pixelCounter <= stopLed; pixelCounter++) {
			setPixel(physicalSegment, pixelCounter, background);
		}
		virtualSegment++;
		physicalSegment = virtualToPhysicalSegment(virtualSegment);

		for (matrixCounter = 0; matrixCounter < 5; matrixCounter++) {
			fontMask = 0x01;
			for (pixelCounter = startLed; pixelCounter <= stopLed;
					pixelCounter++) {
				ledBit = 0;
				if (charMatrix[matrixCounter] & fontMask) {
					ledBit = 1;
				}
				fontMask <<= 1;

				if (ledBit == 1) {
					setPixel(physicalSegment, pixelCounter, color);
				} else {
					setPixel(physicalSegment, pixelCounter, background);
				}
			}
			virtualSegment++;
			physicalSegment = virtualToPhysicalSegment(virtualSegment);
		}

		for (pixelCounter = startLed; pixelCounter <= stopLed; pixelCounter++) {
			setPixel(physicalSegment, pixelCounter, background);
		}
		virtualSegment++;
		physicalSegment = virtualToPhysicalSegment(virtualSegment);
		bufferPosition += bufferPositionDelta;
	}
}

static void loadColors(const uint32_t *cPtr) {
	uint8_t index1;
	for (index1 = 0; index1 < (1 << currentNumColorBits); index1++) {
		colorArray[index1] = *(cPtr + index1);
	}
}

static uint16_t virtualToPhysicalSegment(int16_t virtSegment) {
	while (virtSegment >= (int16_t) currentNumSegments) {
		virtSegment -= currentNumSegments;
	}
	while (virtSegment < 0) {
		virtSegment += currentNumSegments;
	}
	return virtSegment;
}

static void setParameters(uint8_t logSegments, uint8_t colorBits,
		uint16_t tdcSegment) {
	segmentTimer->TCTRL = 0;		// Disable PIT will be enabled in tdcISR()
	segmentTimer->TFLG = 1;			// Clear interrupt flag
	rpmTimer->TCTRL = 0;			// Disable PIT
	rpmTimer->TFLG = 1;				// Clear interrupt flag
	tdcInteruptVector = dummy_funct;
	allLedsOff();

	currentLogNumSegments = logSegments;
	currentNumSegments = 1 << currentLogNumSegments;
	currentSegmentMask = currentNumSegments - 1;
	currentNumColorBits = colorBits;
	currentTdcDisplaySegment = tdcSegment;
	currentColorMask = (1 << currentNumColorBits) - 1;
	switch (currentNumColorBits) {
	case 1:
	case 2:
	case 4:
		bitCountLoad = 0x80000000;
		break;

	case 3:
	case 5:
	case 6:
		bitCountLoad = 0x20000000;
		break;

	default:
		break;
	}
	pixelsPerWord = 32 / currentNumColorBits;

	for (uint16_t i = 0; i < maxNumSegments; i++) {
		for (uint16_t j = 0; j < maxColumns; j++) {
			segmentArray[i][j] = 0;
		}
	}
	tdcInteruptVector = tdcIsrInit;
	rpmTimer->TCTRL = 3;							// Enable PIT and interrupt
}

static void allLedsOff() {
	//FastLED.clear();
	uint32_t index1;
	for (index1 = 0; index1 < numLeds; index1++) {
		leds[index1] = CRGB::Black;
	}
	FastLED.show();
}

static void updateLeds() {
	uint32_t currentWord, bitCounter;
	uint32_t index1, index2;
	index2 = 1;
	currentWord = segmentArray[currentDisplaySegment][0];
	bitCounter = bitCountLoad;
	for (index1 = 0; index1 < numLeds; index1++) {
		leds[index1] = colorArray[currentWord & currentColorMask];
		currentWord >>= currentNumColorBits;
		bitCounter >>= currentNumColorBits;
		if (bitCounter == 0) {
			bitCounter = bitCountLoad;
			currentWord = segmentArray[currentDisplaySegment][index2++];
		}
	}
	FastLED.show();
	//ledOnTimer->TCTRL = 3;
	if (currentLogNumSegments < LOG_512_SEGMENTS) {
		allLedsOff();
	}
}

static void setPixel(uint16_t segment, uint16_t pixel, uint32_t value) {
	uint8_t pixelWord, pixelShift;
	uint32_t pixelMask;

	pixelWord = pixel / pixelsPerWord;
	pixelShift = (pixel % pixelsPerWord) * currentNumColorBits;
	pixelMask = currentColorMask << pixelShift;
	value <<= pixelShift;
	value &= pixelMask;

	segmentArray[segment][pixelWord] &= (~pixelMask);
	segmentArray[segment][pixelWord] |= value;
	//offset = segment * maxColumns + pixelWord;
	//*(segmentArray + offset) &= (~pixelMask);
	//*(segmentArray + offset) |= value;
}

static void mainTdcISR() {
	tdcInteruptVector();
}

static void tdcIsrInit(void) {
	// This ISR fires every time blade passes Hall detector (Top Dead Center)
#ifdef DEBUG_MODE
	tdcIsrFire = 2;
#endif  // DEBUG_MODE

	rpmTimer->TCTRL = 0;	// Reset RPM PIT and interrupt
	rpmTimer->TFLG = 1;
	rpmTimer->TCTRL = 3;

	if (++goodRpmCount >= minGoodRpmCount) { // Confirm spinning at good RPM for several revolutions
		tdcInteruptVector = tdcIsrActive;
	}
}

static void tdcIsrActive(void) {
	// This ISR fires every time blade passes Hall detector (Top Dead Center)
#ifdef DEBUG_MODE
	if (lastSegment != currentNumSegments-1) {
		missedSegment = true;
	}
	tdcIsrFire = 1;
#endif  // DEBUG_MODE

	uint32_t currentRpmCounter, newSegmentCounter;

	currentRpmCounter = rpmTimer->CVAL;
	rpmTimer->TCTRL = 0;	// Reset RPM PIT and interrupt
	rpmTimer->TFLG = 1;
	rpmTimer->TCTRL = 3;
	lastRpmTimerReading = currentRpmCounter;

	segmentTimer->TCTRL = 0;
	segmentTimer->TFLG = 1;
	newSegmentCounter = (rpmCycles - currentRpmCounter)
			>> currentLogNumSegments;
	segmentTimer->LDVAL = newSegmentCounter;
	segmentTimer->TCTRL = 3;		// Enable segment PIT and interrupt
	currentDisplaySegment = currentTdcDisplaySegment;
	updateLeds();	// Set LEDs per currentDisplaySegment
	currentDisplaySegment = (currentDisplaySegment + 1) & currentSegmentMask;
}

static void rpmTimerIsr() {
	// This ISR fires if RPM is too low. Stop running the LEDs
#ifdef DEBUG_MODE
	rpmTimerIsrFire = true;
#endif  // DEBUG_MODE

	static const uint32_t errorColors[] = { CRGB::Red, CRGB::Blue };
	uint32_t displayErrorColor;
	segmentTimer->TCTRL = 0;		 //Disable PIT
	segmentTimer->TFLG = 1;
	goodRpmCount = 0;

	// Turn off all LEDs
	//allLedsOff();
	displayErrorColor = errorColors[digitalRead(hallPin) & 0x1];
	for (uint32_t index = 0; index < numLeds - 1; index++) {
		leds[index] = CRGB::Black;
	}
	leds[numLeds - 1] = displayErrorColor;
	FastLED.show();
	tdcInteruptVector = tdcIsrInit;
}

static void segmentTimerIsr() {
	// This ISR fires for every segment position
#ifdef DEBUG_MODE
	lastSegment = currentDisplaySegment;
	segmentTimerIsrFire = true;
#endif  // DEBUG_MODE

	updateLeds();	// Set LEDs per currentDisplaySegment
	currentDisplaySegment = (currentDisplaySegment + 1) & currentSegmentMask;
	if (currentDisplaySegment == currentTdcDisplaySegment) {
		// Shut down PIT since tdcDisplaySegment is displayed by tdcISR()
		// Wait for tdcISR() to re-enable
		segmentTimer->TCTRL = 0;
		segmentTimer->TFLG = 1;
	}
}

static void ledOffISR() {
	// Shut off LEDS a few uS after lit by segment timer
	ledOnTimer->TCTRL = 0;
	ledOnTimer->TFLG = 1;
	allLedsOff();
}

static void dummy_funct() {
}

void pit0_isr() {
	PIT_TFLG0 = 1;
	funct_table[0]();
}

void pit1_isr() {
	PIT_TFLG1 = 1;
	funct_table[1]();
}

void pit2_isr() {
	PIT_TFLG2 = 1;
	funct_table[2]();
}

void pit3_isr() {
	PIT_TFLG3 = 1;
	funct_table[3]();
}

#ifdef DEBUG_MODE
void TeensyPOV::debugPrint() {
	uint32_t localLast;
	if (missedSegment) {
		localLast = lastSegment;
		missedSegment = false;
		Serial.print("Missed Segment = ");
		Serial.println(localLast);
	}

	uint8_t localTdcIsrFire, localGoodRpmCount;
	uint32_t localSegmentLoadValue;
	int16_t localCurrentDisplaySegment;

	if (tdcIsrFire) {
		noInterrupts()
		;
		localTdcIsrFire = tdcIsrFire;
		localGoodRpmCount = goodRpmCount;
		localSegmentLoadValue = segmentTimer->LDVAL;
		tdcIsrFire = 0;
		interrupts()
		;

		Serial.print("TDC ISR Fired, ");
		if (localTdcIsrFire == 1) {
			Serial.print("Displayed Segment: ");
			Serial.print(currentTdcDisplaySegment);
			Serial.print(", New Segment Counter Load Value: ");
			Serial.println(localSegmentLoadValue);
		} else {
			Serial.print("Incremented goodRpmCount to: ");
			Serial.println(localGoodRpmCount);
		}
		Serial.println();
	}

	if (rpmTimerIsrFire) {
		noInterrupts()
		;
		rpmTimerIsrFire = false;
		interrupts()
		;
		Serial.println("RPM Timer ISR Fired, All LEDs Off, goodRpmCount reset");
		Serial.println();
	}

	if (segmentTimerIsrFire) {
		noInterrupts()
		;
		segmentTimerIsrFire = false;
		localCurrentDisplaySegment = currentDisplaySegment;
		interrupts()
		;
		localCurrentDisplaySegment--;
		if (localCurrentDisplaySegment < 0) {
			localCurrentDisplaySegment = currentNumSegments - 1;
		}
		Serial.print("Segment Timer ISR Fired, Displayed Segment: ");
		Serial.println(localCurrentDisplaySegment);
		Serial.println();
	}
}
#endif  // DEBUG_MODE
