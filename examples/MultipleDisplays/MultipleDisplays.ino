#include <Arduino.h>
#include "TeensyPovDisplay.h"

#define NUM_LEDS 36

extern const LedArrayStruct pictureStruct;

void switchDisplay(TeensyPovDisplay *);
void loadRose(TeensyPovDisplay *);
void loadLimacons(TeensyPovDisplay *);
void startRpmUpdateTimer(TeensyPovDisplay *);
void updateRpm(TeensyPovDisplay *);

const uint8_t clockPin = 13;
const uint8_t dataPin = 11;
const uint8_t hallPin = 3;
const uint8_t numColorBits = TeensyPOV::COLOR_BITS_4;
const uint8_t numDisplays = 6;
const uint16_t tdcSegment = 0;
const uint32_t numLeds = NUM_LEDS;
CRGB leds[numLeds];
uint8_t currentDisplay = 0;

const uint32_t palette[] = { CRGB::Black, CRGB::Fuchsia, CRGB::Red,
		CRGB::Yellow, CRGB::Green, CRGB::Cyan, CRGB::Blue, CRGB::Purple,
		CRGB::Salmon, CRGB::YellowGreen, CRGB::Turquoise, CRGB::SlateBlue,
		CRGB::OliveDrab, CRGB::Navy, CRGB::LightPink, CRGB::DarkOrchid };

TeensyPovDisplay display[numDisplays];

char charBuffer[10];

const DisplayStringSpec stringArray_0[] =
		{ "SPONGEBOB", BOTTOM, 35, 8, 6, true };
const uint8_t numStrings_0 = sizeof(stringArray_0) / sizeof(DisplayStringSpec);

const DisplayStringSpec stringArray_1[] = { { "Eat at Joes", TOP, 35, 6, 0,
		false }, { "Restaurant", TOP, 25, 2, 0, false }, { "Great Value", BOTTOM,
		25, 5, 0, true }, { "Great Food", BOTTOM, 35, 1, 0, true } };
const uint8_t numStrings_1 = sizeof(stringArray_1) / sizeof(DisplayStringSpec);

const DisplayStringSpec stringArray_2[] = { { "HELLO", TOP, 35, 2, 0, false }, {
		"WORLD", TOP, 25, 3, 0, false },
		{ "POV", BOTTOM, 25, 7, 0, false }, { "Teensy", BOTTOM, 35, 5, 0,
				false } };
const uint8_t numStrings_2 = sizeof(stringArray_2) / sizeof(DisplayStringSpec);

const DisplayStringSpec stringArray_3[] = { { "Fan", TOP, 35, 2, 0, false }, {
		"Speed", TOP, 25, 2, 0, false }, { charBuffer, BOTTOM, 25, 6, 0, true },
		{ "RPM", BOTTOM, 35, 6, 0, true } };
const uint8_t numStrings_3 = sizeof(stringArray_3) / sizeof(DisplayStringSpec);

void setup() {
	Serial.begin(115200);
	delay(1000);

	Serial.println("Starting POV - Test 1");
	FastLED.addLeds<APA102, dataPin, clockPin, BGR, DATA_RATE_MHZ(24)>(leds,
			numLeds);

	TeensyPOV::povSetup(hallPin, leds, numLeds);

	display[0].load(&pictureStruct, stringArray_0, numStrings_0);
	display[0].setTiming(10000, 0, 0);
	display[0].setExpireCallback(switchDisplay);

	display[1].load(stringArray_1, numStrings_1);
	display[1].setDisplay(TeensyPOV::LOG_256_SEGMENTS, numColorBits, tdcSegment,
			palette);
	display[1].setTiming(10000, 0, 0);
	display[1].setExpireCallback(switchDisplay);

	display[2].load(stringArray_2, numStrings_2);
	display[2].setDisplay(TeensyPOV::LOG_128_SEGMENTS, numColorBits, tdcSegment,
			palette);
	display[2].setTiming(15000, 100, -1);
	display[2].setExpireCallback(switchDisplay);

	display[3].load(stringArray_3, numStrings_3);
	display[3].setDisplay(TeensyPOV::LOG_128_SEGMENTS, numColorBits, tdcSegment,
			palette);
	display[3].setTiming(10000, 0, 0);
	display[3].setActivationCallback(startRpmUpdateTimer);
	display[3].setUpdateCallback(updateRpm);
	display[3].setExpireCallback(switchDisplay);

	display[4].load();
	display[4].setDisplay(TeensyPOV::LOG_128_SEGMENTS, numColorBits, tdcSegment,
			palette);
	display[4].setTiming(10000, 30, 1);
	display[4].setActivationCallback(loadLimacons);
	display[4].setExpireCallback(switchDisplay);

	display[5].load();
	display[5].setDisplay(TeensyPOV::LOG_512_SEGMENTS, numColorBits, tdcSegment,
			palette);
	display[5].setTiming(10000, 75, -1);
	display[5].setActivationCallback(loadRose);
	display[5].setExpireCallback(switchDisplay);

	while (!TeensyPOV::rpmGood()) {
	}

	display[currentDisplay].activate();
}

void loop() {
	display[currentDisplay].update();
}

void switchDisplay(TeensyPovDisplay *ptr) {
	currentDisplay++;
	currentDisplay %= numDisplays;
	display[currentDisplay].activate();
}

void loadLimacons(TeensyPovDisplay *ptr) {
	const uint8_t colorSelect = (1 << numColorBits) - 1;
	const float twicePi = 2.0 * 3.14159;
	const float halfPi = 3.14159 / 2.0;
	const float shapeFactor = 2.0;
	const float amplitude = numLeds / (shapeFactor + 1.0) - 1;
	float angle, radius;
	uint8_t pixel, color;
	uint16_t segment, displaySegment, numSegments;

	numSegments = TeensyPOV::getNumSegments();
	for (segment = 0; segment < numSegments; segment++) {
		angle = -(float) segment / numSegments;
		angle *= twicePi;
		angle += halfPi;
		radius = amplitude * (1.0 - shapeFactor * cos(angle));
		if (radius > 0) {
			displaySegment = segment;
		} else {
			displaySegment = segment + numSegments / 2;
			if (displaySegment >= numSegments) {
				displaySegment -= numSegments;
			}
			radius = -radius;
		}
		pixel = radius + 0.5;
		color = segment % colorSelect + 1;
		TeensyPOV::setPixel(displaySegment, pixel, color);
	}
}

void loadRose(TeensyPovDisplay *ptr) {
	const uint8_t colorSelect = (1 << numColorBits) - 1;
	const float twicePi = 2.0 * 3.14159;
	const float halfPi = 3.14159 / 2.0;
	const float shapeFactor = 4.0;
	const float amplitude = numLeds - 1;
	float angle, radius;
	uint8_t pixel, color;
	uint16_t segment, displaySegment, numSegments;

	numSegments = TeensyPOV::getNumSegments();
	for (segment = 0; segment < numSegments; segment++) {
		angle = -(float) segment / numSegments;
		angle *= twicePi;
		angle += halfPi;
		radius = amplitude * cos(shapeFactor * angle);
		if (radius > 0) {
			displaySegment = segment;
		} else {
			displaySegment = segment + numSegments / 2;
			if (displaySegment >= numSegments) {
				displaySegment -= numSegments;
			}
			radius = -radius;
		}
		pixel = radius + 0.5;
		color = segment % colorSelect + 1;
		TeensyPOV::setPixel(displaySegment, pixel, color);
	}
}

uint32_t rpmUpdateTimer;
const uint32_t rpmUpdate = 50;

void startRpmUpdateTimer(TeensyPovDisplay *ptr) {
	rpmUpdateTimer = millis() - rpmUpdate;
}

void updateRpm(TeensyPovDisplay *ptr) {
	static const uint32_t rpmConversion = F_BUS * 60UL;
	int rpm;
	if (millis() - rpmUpdateTimer >= rpmUpdate) {
		rpmUpdateTimer += rpmUpdate;
		rpm = rpmConversion / TeensyPOV::getLastRotationCount();
		itoa(rpm, charBuffer, 10);
		ptr->refresh();
	}
}
