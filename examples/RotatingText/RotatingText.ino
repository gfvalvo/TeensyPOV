#include <Arduino.h>
#include "TeensyPovDisplay.h"

#define NUM_LEDS 36

const uint8_t clockPin = 13;
const uint8_t dataPin = 11;
const uint8_t hallPin = 21;
const uint8_t numColorBits = TeensyPOV::COLOR_BITS_2;
uint8_t tdcSegment = 0;
const uint16_t logNumSegements = TeensyPOV::LOG_128_SEGMENTS;
const uint16_t numLeds = NUM_LEDS;

CRGB leds[numLeds];

const uint32_t palette[] = { CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Blue };

TeensyPovDisplay display;

const DisplayStringSpec stringArray[] = { { "HELLO", TOP, 35, 1, 0, false }, {
		"WORLD", BOTTOM, 35, 3, 0, true } };
const uint8_t numStrings = sizeof(stringArray) / sizeof(DisplayStringSpec);

void setup() {
	Serial.begin(115200);
	delay(1000);

	Serial.println("Starting POV");
	FastLED.addLeds<APA102, dataPin, clockPin, BGR, DATA_RATE_MHZ(24)>(leds,
			numLeds);

	TeensyPOV::povSetup(hallPin, leds, numLeds);

	display.load(stringArray, numStrings);
	display.setDisplay(logNumSegements, numColorBits, tdcSegment, palette);
	display.setTiming(0, 60, -1);

	while (!TeensyPOV::rpmGood()) {
	}
	display.activate();
}

void loop() {
	display.update();
}
