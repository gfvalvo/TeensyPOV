#include "GFV_POV_01.h"

static const uint32_t numSegments = 64;
static const uint32_t numColorBits = 1;
static const uint32_t tdcDisplaySegment = 0;

static const uint16_t bitsPerSegment = NUM_LEDS * numColorBits;
static const uint32_t bitsPerWord =
		((numColorBits == 3) || (numColorBits == 5) || (numColorBits == 6)) ?
				30 : 32;
static const uint32_t remainderBits = bitsPerSegment % bitsPerWord;
static const uint32_t extraWord = (remainderBits != 0) ? 1 : 0;
static const uint32_t numColumns = (bitsPerSegment / bitsPerWord) + extraWord;
static const uint32_t crossColors[1 << numColorBits] =
		{ 0x00000000,
		0x00FFFFFF };
static const uint32_t crossArray64[numSegments][numColumns] = { { 0xFFFFFFFF,
		0xFFFFFFFF }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0xFFFFFFFF, 0xFFFFFFFF },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0xFFFFFFFF, 0xFFFFFFFF }, { 0, 0 }, { 0, 0 }, { 0,
				0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0xFFFFFFFF, 0xFFFFFFFF }, { 0, 0 }, { 0, 0 }, { 0, 0 },
		{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, {
				0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };

const LedArrayStruct crossStruct64 = { &crossArray64[0][0], &crossColors[0],
		numColorBits, numSegments, numColumns, tdcDisplaySegment };
