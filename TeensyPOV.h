/*
 * TeensyPOV.h
 *
 *  Created on: Apr 28, 2018
 *      Author: GFV
 */

#ifndef TEENSYPOV_H_
#define TEENSYPOV_H_

//#define SIMULATE_RPM
//#define DEBUG_MODE

#include <Arduino.h>
#define FASTLED_INTERNAL
#include "FastLED.h"

enum TextPosition {
	TOP, BOTTOM
};

struct LedArrayStruct {
	const uint32_t *array;
	const uint32_t *colors;
	uint32_t numColorBits;
	uint8_t logNumSegments;
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
	friend class TeensyPovDisplay;
public:
	static const uint8_t LOG_2_SEGMENTS = 1;
	static const uint8_t LOG_4_SEGMENTS = 2;
	static const uint8_t LOG_8_SEGMENTS = 3;
	static const uint8_t LOG_16_SEGMENTS = 4;
	static const uint8_t LOG_32_SEGMENTS = 5;
	static const uint8_t LOG_64_SEGMENTS = 6;
	static const uint8_t LOG_128_SEGMENTS = 7;
	static const uint8_t LOG_256_SEGMENTS = 8;
	static const uint8_t LOG_512_SEGMENTS = 9;

	static const uint8_t COLOR_BITS_1 = 1;
	static const uint8_t COLOR_BITS_2 = 2;
	static const uint8_t COLOR_BITS_4 = 4;
	static const uint8_t COLOR_BITS_8 = 8;

	static bool povSetup(uint8_t, CRGB *, uint8_t);
	static bool rpmGood(void);
	static uint16_t getNumSegments(void);
	static uint32_t getLastRotationCount(void);
	static void setPixel(uint16_t, uint16_t, uint32_t);
	static void (*funct_table[4])();
	static void (*tdcInteruptVector)();

#ifdef DEBUG_MODE
	static void debugPrint(void);
#endif 		//DEBUG_MODE

private:
	static void dummy_funct(void);
	static void rpmTimerIsr(void);
	static void segmentTimerIsr(void);
	static void tdcIsrInit(void);
	static void tdcIsrActive(void);
	static void updateLeds(void);
	static void allLedsOff(void);
	static void loadPattern(const LedArrayStruct *);
	static void loadColors(const uint32_t *);
	static void setParameters(uint8_t, uint8_t, uint16_t);
	static void loadString(const char *, TextPosition, uint8_t, uint8_t,
			uint8_t, bool);

#ifndef SIMULATE_RPM
	static const uint32_t maxRevolutionPeriod = 100000UL; // Only run LEDs when > 10 revs / sec (600 RPM)
#else
	static const uint32_t maxRevolutionPeriod = 30000000UL;
#endif  //SIMULATE_RPM

	static const uint32_t rpmCycles = (F_BUS / 1000000UL) * maxRevolutionPeriod
			- 1;

	static const uint32_t maxNumLeds = 48;
	static const uint32_t maxNumColorBits = COLOR_BITS_8;
	static const uint32_t bitCountLoad = 0x80000000;
	static const uint32_t maxNumSegments = 1 << LOG_512_SEGMENTS;
	static const uint32_t bitsPerSegment = maxNumLeds * maxNumColorBits;
	static const uint32_t bitsPerWord = 32;
	static const uint32_t maxColumns = (bitsPerSegment / bitsPerWord);
	static const uint8_t maxTextChars = maxNumSegments / (2 * 7);
	static const uint8_t minGoodRpmCount = 2;

	static uint8_t pixelsPerWord;
	static uint32_t numLeds;
	static CRGB *leds;

	static KINETISK_PIT_CHANNEL_t *rpmTimer;
	static KINETISK_PIT_CHANNEL_t *segmentTimer;
#ifdef SIMULATE_RPM
	static const uint32_t tdcSimulatorCycles = (F_BUS / 1000000UL) * 50000UL
			- 1;
	static KINETISK_PIT_CHANNEL_t *tdcSimulator;
#endif  // SIMULATE_RPM

	volatile static uint8_t currentLogNumSegments;
	volatile static uint32_t segmentArray[maxNumSegments][maxColumns];
	volatile static uint32_t colorArray[1 << maxNumColorBits];
	volatile static uint32_t currentNumColorBits;
	volatile static uint32_t currentNumSegments;
	volatile static uint32_t currentSegmentMask;
	volatile static uint32_t currentColorMask;
	volatile static uint32_t goodRpmCount;
	volatile static uint32_t currentDisplaySegment;
	volatile static uint32_t currentTdcDisplaySegment;
	volatile static uint32_t updateTdcDisplaySegment;
	volatile static uint32_t lastRpmTimerReading;
	volatile static uint8_t hallPin;
#ifdef DEBUG_MODE
	volatile static bool segmentTimerIsrFire;
	volatile static bool rpmTimerIsrFire;
	volatile static bool missedSegment;
	volatile static uint8_t tdcIsrFire;
	volatile static uint32_t lastSegment;
#endif  // DEBUG_MODE
};

#endif /* TEENSYPOV_H_ */
