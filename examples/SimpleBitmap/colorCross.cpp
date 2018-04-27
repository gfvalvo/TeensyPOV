#include <stdint.h>
#include "TeensyPOV.h"
#include "SimpleBitmap.h"

static const uint32_t logNumSegments = 7;
static const uint32_t numSegments = 1 << logNumSegments;
static const uint32_t numColorBits = 3;
static const uint32_t tdcDisplaySegment = 0;

static const uint16_t bitsPerSegment = NUM_LEDS * numColorBits;
static const uint32_t bitsPerWord =
		((numColorBits == 3) || (numColorBits == 5) || (numColorBits == 6)) ?
				30 : 32;
static const uint32_t remainderBits = bitsPerSegment % bitsPerWord;
static const uint32_t extraWord = (remainderBits != 0) ? 1 : 0;
static const uint32_t numColumns = (bitsPerSegment / bitsPerWord) + extraWord;
static const uint32_t crossColors[1 << numColorBits] =
		{ 0x00000000, 0x00555555,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 0x00000000, 0x00000000 };
static const uint32_t crossArray[numSegments][numColumns] = { { 0x09249249,
		0x09249249, 0x09249249, 0x09249249 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
		0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0,
		0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
		0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0,
		0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
		0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0,
		0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
		0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0x12492492, 0x12492492, 0x12492492,
		0x12492492 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0,
		0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
		0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0,
		0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
		0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0,
		0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
		0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0,
		0 }, { 0x1B6DB6DB, 0x1B6DB6DB, 0x1B6DB6DB, 0x1B6DB6DB }, { 0, 0, 0, 0 },
		{ 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0,
				0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0,
				0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0,
				0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0,
				0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0,
				0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0,
				0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0,
				0, 0 }, { 0, 0, 0, 0 }, { 0x24924924, 0x24924924, 0x24924924,
				0x24924924 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
				0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
				0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
				0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
				0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
				0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
				0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, {
				0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

const LedArrayStruct colorCrossStruct = { &crossArray[0][0], &crossColors[0],
		numColorBits, logNumSegments, numColumns, tdcDisplaySegment };
