#include "TeensyPovDisplay.h"

#define NUM_LEDS 36

extern const LedArrayStruct colorCrossStruct;

const uint8_t clockPin = 13;
const uint8_t dataPin = 11;
const uint8_t hallPin = 3;
const uint32_t numLeds = NUM_LEDS;

CRGB leds[numLeds];
TeensyPovDisplay display;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting POV - Test 1");
  FastLED.addLeds<APA102, dataPin, clockPin, BGR, DATA_RATE_MHZ(24)>(leds,
      numLeds);

  TeensyPOV::povSetup(hallPin, leds, numLeds);
  display.load(&colorCrossStruct);
  display.activate();
}

void loop() {}