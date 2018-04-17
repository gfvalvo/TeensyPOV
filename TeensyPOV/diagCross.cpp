#include "GFV_POV_01.h"

static const uint32_t numSegments = 128;
static const uint32_t numColorBits = 1;
static const uint32_t tdcDisplaySegment = 0;

//static const uint16_t bitsPerSegment = numLeds * numColorBits;
static const uint32_t bitsPerSegment = NUM_LEDS * numColorBits;
static const uint32_t bitsPerWord =
		((numColorBits == 3) || (numColorBits == 5) || (numColorBits == 6)) ?
				30 : 32;
static const uint32_t remainderBits = bitsPerSegment % bitsPerWord;
static const uint32_t extraWord = (remainderBits != 0) ? 1 : 0;
static const uint32_t numColumns = (bitsPerSegment / bitsPerWord) + extraWord;
static const uint32_t diagCrossColors[1 << numColorBits] = { 0x00000000,
		0x000000FF };
static const uint32_t diagCrossArray[numSegments][numColumns] = { { 0, 0 }, { 0,
		0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0xFFFFFFFF, 0xFFFFFFFF }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0xFFFFFFFF, 0xFFFFFFFF }, { 0, 0 }, { 0,
				0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0xFFFFFFFF, 0xFFFFFFFF },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0xFFFFFFFF,
				0xFFFFFFFF }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 } };

const LedArrayStruct diagCrossStruct = { &diagCrossArray[0][0],
		&diagCrossColors[0], numColorBits, numSegments, numColumns,
		tdcDisplaySegment };
