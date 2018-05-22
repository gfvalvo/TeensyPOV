/*
 * TeensyPOV.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: GFV
 */
#include "TeensyPOV.h"
#include "textCharacters.h"

static void mainTdcISR(void);

uint8_t TeensyPOV::pixelsPerWord;
uint32_t TeensyPOV::numLeds;
CRGB * TeensyPOV::leds;

KINETISK_PIT_CHANNEL_t * TeensyPOV::rpmTimer;
KINETISK_PIT_CHANNEL_t * TeensyPOV::segmentTimer;
#ifdef SIMULATE_RPM
KINETISK_PIT_CHANNEL_t * TeensyPOV::tdcSimulator;
#endif  // SIMULATE_RPM

void (*TeensyPOV::funct_table[4])() = {TeensyPOV::dummy_funct, TeensyPOV::dummy_funct, TeensyPOV::dummy_funct, TeensyPOV::dummy_funct};
void (*TeensyPOV::tdcInteruptVector)() = TeensyPOV::dummy_funct;

volatile uint8_t TeensyPOV::currentLogNumSegments = 1;
volatile uint32_t TeensyPOV::segmentArray[maxNumSegments][maxColumns];
volatile uint32_t TeensyPOV::colorArray[1 << maxNumColorBits];
volatile uint32_t TeensyPOV::currentNumColorBits = 0;
volatile uint32_t TeensyPOV::currentNumSegments = 1 << currentLogNumSegments;
volatile uint32_t TeensyPOV::currentSegmentMask = currentNumSegments - 1;
volatile uint32_t TeensyPOV::currentColorMask;
volatile uint32_t TeensyPOV::goodRpmCount;
volatile uint32_t TeensyPOV::currentDisplaySegment;
volatile uint32_t TeensyPOV::currentTdcDisplaySegment = 0;
volatile uint32_t TeensyPOV::updateTdcDisplaySegment = currentTdcDisplaySegment;
volatile uint32_t TeensyPOV::lastRpmTimerReading;
volatile uint8_t TeensyPOV::hallPin;
#ifdef DEBUG_MODE
volatile bool TeensyPOV::segmentTimerIsrFire = false;
volatile bool TeensyPOV::rpmTimerIsrFire = false;
volatile bool TeensyPOV::missedSegment = false;
volatile uint8_t TeensyPOV::tdcIsrFire = 0;
volatile uint32_t TeensyPOV::lastSegment;
#endif  // DEBUG_MODE

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

	const uint8_t rpmTimerInterruptPriority = 128;
	const uint8_t segmentTimerInterruptPriority = 128;

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

void TeensyPOV::loadPattern(const LedArrayStruct *patternStruct) {
	uint32_t row, column;

	for (row = 0; row < currentNumSegments; row++) {
		for (column = 0; column < patternStruct->columns; column++) {
			segmentArray[row][column] = *(patternStruct->array
					+ row * patternStruct->columns + column);
		}
	}
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

uint16_t TeensyPOV::getNumSegments() {
	return currentNumSegments;
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

void TeensyPOV::setPixel(uint16_t segment, uint16_t pixel, uint32_t value) {
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
	uint8_t pixelWord, pixelShift;
	uint32_t pixelMask;

	pixelWord = pixel / pixelsPerWord;
	pixelShift = (pixel % pixelsPerWord) * currentNumColorBits;
	pixelMask = currentColorMask << pixelShift;
	value <<= pixelShift;
	value &= pixelMask;

	segmentArray[segment][pixelWord] &= (~pixelMask);
	segmentArray[segment][pixelWord] |= value;
}



void TeensyPOV::loadColors(const uint32_t *cPtr) {
	uint16_t index1;
	for (index1 = 0; index1 < (1 << currentNumColorBits); index1++) {
		colorArray[index1] = *(cPtr + index1);
	}
}

void TeensyPOV::loadString(const char *string, TextPosition pos, uint8_t topLed,
		uint8_t color, uint8_t background, bool invert) {
	char charBuffer[maxTextChars + 1], *bufferPosition;
	uint8_t len, fontMask, ledBit;
	//uint8_t padPosition;
	uint8_t charMatrix[5];
	int8_t bufferPositionDelta;
	uint16_t currentMaxChars, stopLed, startLed, physicalSegment;
	uint16_t charCounter, pixelCounter, matrixCounter;
	uint16_t virtualSegment;

	if (topLed >= numLeds) {
		return;
	}

	if (topLed < 6) {
		return;
	}

	stopLed = topLed;
	startLed = stopLed - 6;

	memset(charBuffer, '\0', maxTextChars);

	currentMaxChars = currentNumSegments / (2 * 7);
	len = strlen(string);
	if (len > currentMaxChars) {
		len = currentMaxChars;
	}
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
	physicalSegment = virtualSegment & currentSegmentMask;

	for (charCounter = 0; charCounter < len; charCounter++) {
	textCharacters::getMatrix(*bufferPosition, charMatrix, invert);

		for (pixelCounter = startLed; pixelCounter <= stopLed; pixelCounter++) {
			setPixel(physicalSegment, pixelCounter, background);
		}
		virtualSegment++;
		physicalSegment = virtualSegment & currentSegmentMask;

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
			physicalSegment = virtualSegment & currentSegmentMask;
		}

		for (pixelCounter = startLed; pixelCounter <= stopLed; pixelCounter++) {
			setPixel(physicalSegment, pixelCounter, background);
		}
		virtualSegment++;
		physicalSegment = virtualSegment & currentSegmentMask;
		bufferPosition += bufferPositionDelta;
	}
}

void TeensyPOV::setParameters(uint8_t logSegments, uint8_t colorBits,
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
	updateTdcDisplaySegment = currentTdcDisplaySegment;
	currentColorMask = (1 << currentNumColorBits) - 1;
	pixelsPerWord = 32 / currentNumColorBits;

	for (uint16_t i = 0; i < maxNumSegments; i++) {
		for (uint16_t j = 0; j < maxColumns; j++) {
			segmentArray[i][j] = 0;
		}
	}
	tdcInteruptVector = tdcIsrInit;
	rpmTimer->TCTRL = 3;							// Enable PIT and interrupt
}

void TeensyPOV::allLedsOff() {
	uint32_t index1;
	for (index1 = 0; index1 < numLeds; index1++) {
		leds[index1] = CRGB::Black;
	}
	FastLED.show();
}


void TeensyPOV::updateLeds() {
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
	if (currentLogNumSegments < LOG_512_SEGMENTS) {
		allLedsOff();
	}
}

static void mainTdcISR() {
	TeensyPOV::tdcInteruptVector();
}

void TeensyPOV::tdcIsrInit() {
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

void TeensyPOV::tdcIsrActive() {
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
	currentTdcDisplaySegment = updateTdcDisplaySegment;
	currentDisplaySegment = currentTdcDisplaySegment;
	updateLeds();	// Set LEDs per currentDisplaySegment
	currentDisplaySegment = (currentDisplaySegment + 1) & currentSegmentMask;
}

void TeensyPOV::rpmTimerIsr() {
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

void TeensyPOV::segmentTimerIsr() {
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

void TeensyPOV::dummy_funct() {
}

void pit0_isr() {
	PIT_TFLG0 = 1;
	TeensyPOV::funct_table[0]();
}

void pit1_isr() {
	PIT_TFLG1 = 1;
	TeensyPOV::funct_table[1]();
}

void pit2_isr() {
	PIT_TFLG2 = 1;
	TeensyPOV::funct_table[2]();
}

void pit3_isr() {
	PIT_TFLG3 = 1;
	TeensyPOV::funct_table[3]();
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
