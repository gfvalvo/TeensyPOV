#include "GFV_POV_01.h"

void loadCardioid(void);
void startRpmUpdateTimer(void);
void updateRpm(void);

const uint8_t clockPin = 13;
const uint8_t dataPin = 11;
const uint8_t hallPin = 3;
const uint32_t numLeds = NUM_LEDS;
CRGB leds[numLeds];

const uint32_t palette[] = { CRGB::Black, CRGB::Fuchsia, CRGB::Red,
		CRGB::Yellow, CRGB::Green, CRGB::Cyan, CRGB::Blue, CRGB::Purple };

TeensyPOV display[5];

char charBuffer[10];

const DisplayStringSpec stringArray_0[] = { "DINY", BOTTOM, 35, 6, 7, true };
const uint8_t numStrings_0 = sizeof(stringArray_0) / sizeof(DisplayStringSpec);

const DisplayStringSpec stringArray_1[] = { { "GREGORY", TOP, 35, 6, 0, false },
		{ "VALVO", TOP, 25, 2, 0, false }, { "DIANE", BOTTOM, 25, 5, 0, true },
		{ "VALVO", BOTTOM, 35, 1, 0, true } };
const uint8_t numStrings_1 = sizeof(stringArray_1) / sizeof(DisplayStringSpec);

const DisplayStringSpec stringArray_2[] = { { "JESSICA", TOP, 35, 2, 0, false },
		{ "VALVO", TOP, 25, 3, 0, false }, { "VALVO", BOTTOM, 25, 7, 0, false },
		{ "JAMIE", BOTTOM, 35, 5, 0, false } };
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

	//display[0].load(&faceStruct, stringArray_0, numStrings_0);
	display[0].load(&faceStruct, stringArray_0, numStrings_0);
	display[0].setTiming(5000, 0, 0);

	display[1].setDisplay(128, 3, 0, palette);
	display[1].load(stringArray_1, numStrings_1);
	display[1].setTiming(5000, 0, 0);

	display[2].setDisplay(128, 3, 0, palette);
	display[2].load(stringArray_2, numStrings_2);
	display[2].setTiming(5000, 20, -1);

	display[3].setDisplay(128, 3, 0, palette);
	display[3].load(stringArray_3, numStrings_3);
	display[3].setTiming(5000, 0, 0);
	display[3].setActivationCallback(startRpmUpdateTimer);
	display[3].setUpdateCallback(updateRpm);

	display[4].setDisplay(128, 3, 0, palette);
	display[4].load();
	display[4].setTiming(7000, 30, 1);
	display[4].setActivationCallback(loadCardioid);

	while (!TeensyPOV::rpmGood()) {
	}

	display[0].activate();
}

void loop() {
	static uint8_t numDisplays = 5, currentDisplay = 0;


	if (display[currentDisplay].update()) {
		currentDisplay++;
		currentDisplay %= numDisplays;
		display[currentDisplay].activate();
	}

	if (currentDisplay == 3) {

	}

#ifdef DEBUG_MODE
	TeensyPOV::debugPrint();
#endif

}

void loadCardioid() {
	const float twicePi = 2.0 * 3.14159;
	const float halfPi = 3.14159 / 2.0;
	const float amplitude = numLeds / 2.0;
	float angle, radius;
	uint8_t pixel, color;
	const uint16_t numSegments = 128;
	uint16_t segment;
	for (segment = 0; segment < numSegments; segment++) {
		angle = -(float) segment / numSegments;
		angle *= twicePi;
		angle += halfPi;
		radius = amplitude * (1 - cos(angle));
		pixel = radius;
		color = segment % 7 + 1;
		TeensyPOV::setLed(segment, pixel, color);
	}
}

uint32_t rpmUpdateTimer;
const uint32_t rpmUpdate = 50;

void startRpmUpdateTimer() {
	rpmUpdateTimer = millis() - rpmUpdate;
}

void updateRpm() {
	static const uint32_t rpmConversion = F_BUS * 60UL;
	int rpm;
	if (millis() - rpmUpdateTimer >= rpmUpdate) {
		rpmUpdateTimer += rpmUpdate;
		rpm = rpmConversion / TeensyPOV::getLastRotationCount();
		itoa(rpm, charBuffer, 10);
		display[3].activate(false);
	}
}
