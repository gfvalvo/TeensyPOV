#include <Arduino.h>
#include "TeensyPovDisplay.h"

#define NUM_LEDS 36

extern const LedArrayStruct pictureStruct;

void switchDisplay(TeensyPOV *);
void loadRose(TeensyPOV *);
void loadLimacons(TeensyPOV *);
void startRpmUpdateTimer(TeensyPOV *);
void updateRpm(TeensyPOV *);

const uint8_t clockPin = 13;
const uint8_t dataPin = 11;
const uint8_t hallPin = 3;
const uint8_t numColorBits = 2;
uint8_t tdcSegment = 0;
const uint16_t logNumSegements = 7;
const uint16_t numLeds = NUM_LEDS;

CRGB leds[numLeds];

const uint32_t palette[] = { CRGB::Black, CRGB::Red, CRGB::Green, CRGB::Blue};

TeensyPovDisplay display;

const DisplayStringSpec stringArray[] = { { "HELLO", TOP, 35, 1, 0,
		false }, { "WORLD", BOTTOM, 35, 3, 0, true } };
const uint8_t numStrings = sizeof(stringArray) / sizeof(DisplayStringSpec);

void setup() {
	Serial.begin(115200);
	delay(1000);

	Serial.println("Starting POV - Test 1");
	FastLED.addLeds<APA102, dataPin, clockPin, BGR, DATA_RATE_MHZ(24)>(leds,
			numLeds);

	TeensyPOV::povSetup(hallPin, leds, numLeds);

	display.load(stringArray, numStrings);
	display.setDisplay(logNumSegements, numColorBits, tdcSegment, palette);

	while (!TeensyPOV::rpmGood()) {
	}
	display.activate();
}

void loop() {}
