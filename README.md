# TeensyPOV:

## POV Project on Teensy 3.2 - WORK IN PROGRESS

Library for POV (Persistence of Vision) display using Teensy 3.2 board and Adafuit DotStar LEDs (APA102). Descriptions of the hardware (including schematics and PWB) are in the Hardware folder.
The library is configured to support up to 48 LEDs in the string and up to 512 angular positions (~0.7 degree resolution). These positions are referred to as "segments" in the code and description below.
## Sample Images:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/DSC_5983.JPG)
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/DSC_5985.JPG)
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/DSC_5988.JPG)
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/DSC_5991.JPG)
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/DSC_5995.JPG)
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/DSC_6008.JPG)
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/DSC_6011.JPG)
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/DSC_6020.JPG)

[Video of POV in Action](https://youtu.be/CiRSouhlJos)
Note that interaction between the video's frame rate and the LED refresh rate causes some artifacts that aren't really there when you view the POV in person.

## Library Description:
Code in this library directly manipulates the Teensy processor's Programmable Interrupt Timers. Thus, it is incompatible with any code that uses these timers directly or indirectly via the Teensyduino IntervalTimer class.

All of the following class methods are demonstrated in the example sketches.
### Classes:
#### Class TeensyPOV
This class interfaces with the hardware and controls the LEDs to produce the POV display. All methods in this class are static. Thus there is no need to instantiate an object of this type.
#### Public TeensyPOV Members Functions:
****Initialize the T3.2 hardware needed for the POV Display****

    bool povSetup(uint8_t hPin, CRGB *ledPtr, uint8_t num)
**Arguments:**
 - **uint8_t hPin** - Hardware pin that the Hall Effect Sensor is connected to.
 - **CRGB \*ledPtr** - Pointer to an array of FastLED CRGB objects. These objects should be registered with the FastLED library before calling povSetup.
 - **uint8_t num** - Number LEDs in the array
 
****Check if Rotation Speed is High Enough for POV Display****

    bool rpmGood(void) 

****Get Number of Segments in the Currently Active POV Display.****

    uint16_t getNumSegments(void)

****Get Number of Teensy 3.2 PIT Ticks that Occurred During the Blade's last Rotation Period. On a 96 MHz T3.2 there are 48,000,000 Ticks per Second.****

    uint32_t getLastRotationCount(void)

****Set the color of an LED directly on the display.****

    void setPixel(uint16_t segment, uint16_t pixel, uint32_t value)


**Arguments:**

- **uint16_t segment** - Rotational segment of the LED, counted clockwise from Top Dead Center. Value between 0 and (Number of Segments)-1
- **uint16_t pixel** - Radial position of the LED, 0 = innermost.
- **uint32_t value** - Color of the LED expressed as index into current Palette.

#### Public TeensyPOV Data Members:

Defined constants that can be used in calls to TeensyPovDisplay methods (see **Class TeensyPovDisplay**):
```
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
```
#### Class TeensyPovDisplay
This class allows the user to define POV images to be displayed. There can be multiple instances of this class defining different images. Only one is actively displayed at a time.
#### Public TeensyPovDisplay Members Functions:
****Constructor.****
````
TeensyPovDisplay()
````
****Load an empty TeensyPovDisplay object. Would be used if no bitmap image or text is desired. Pixels would be set by the main program using the setLed() method.****
````
void load(void)
````
****Load a TeensyPovDisplay object with a bit pattern image.****
````
void load(const LedArrayStruct *pattern)
````


**Arguments:**

- **const LedArrayStruct \*pattern** - Pointer to the image structure. The struct pointed to must be static or global. See below for definition of the LedArrayStruct structure.

****Load a TeensyPovDisplay object with text strings.****
````
void load(const DisplayStringSpec *strArray, uint8_t n)
````
**Arguments:**
- **const DisplayStringSpec \*strArray** - Pointer to an array of DisplayStringSpec structures. The array pointed to must be static or global. See below for definition of the DisplayStringSpec structure.
- **uint8_t n** - Number of DisplayStringSpec structures in the array.

****Load a TeensyPovDisplay object with a bit pattern image and text strings.****
````
void load(const LedArrayStruct *pattern, const DisplayStringSpec *strArray, uint8_t n)
````
**Arguments:**
- **const LedArrayStruct \*pattern** - Pointer to the image structure. The struct pointed to must be static or global. See below for definition of the LedArrayStruct structure.
- **const DisplayStringSpec \*strArray** - Pointer to an array of DisplayStringSpec structures. The array pointed to must be static or global. See below for definition of the DisplayStringSpec structure.
- **uint8_t n** - Number of DisplayStringSpec structures in the array.

****Display a TeensyPovDisplay object on the POV LEDs. Resets the duration and rotation timers.****
````
void activate()
````
****Refresh the TeensyPovDisplay object on the POV LEDs after changing text, bit image, palette, etc. Does not reset the duration and rotation timers****
````
void refresh()
````
****Set the POV parameters for a TeensyPovDisplay object that does not contain a bitmap image (i.e. a LedArrayStruct). Call this function after load() and before activate()****
````
void setDisplay(uint8_t logSeg, uint8_t cBits, uint16_t tdc, const uint32_t *colors)
````
**Arguments:**
- **uint8_t logSeg** - Log (base 2) of number of TDC segments. Use static constants defined by class TeensyPOV.
- **uint8_t cBits** - Number of color bits. Use static constants defined by class TeensyPOV.
- **uint16_t tdc** - Segment number to display when rotating blade hits Top Dead Center.
- **const uint32_t \*colors** - Pointer to array defining RGB color pallet. Array must have at least 2 ^ cBits elements. The array pointed to must be static or global.

****Set timing for dynamic TeensyPovDisplay objects. Calling this function is optional, all values are set to their default values by the load() method.****
````
void setTiming(uint32_t duration, uint32_t rotation, int16_t tdcDelta)
````
**Arguments:**
- **uint32_t duration** - Time (in milliseconds) before the object expires. Use zero (default) if object is permanent.
- **uint32_t rotation** - Time (in milliseconds) between changes in the Top Dead Center segment, causes display to rotate. Use zero (default)  for no rotation.
- **int16_t tdcDelta** - Adjustment to Top Dead Center segment every 'rotation' milliseconds. Use negative value for clockwise rotation and positive value for anti-clockwise. Has no effect if 'rotation' is zero.

****Determine if duration of TeensyPovDisplay object has expired and performs specified rotation****
````
bool update()
````
**Returns:** True if display's time has expired.

****Set optional callback function to be called when a TeensyPovDisplay object is activated.****
````
void setActivationCallback(void (*ptr)(TeensyPovDisplay *))
````
**Arguments:**
- **void (\*ptr)(TeensyPOV \*)** - Pointer to the function to be called. The function must take as an argument a pointer to the TeensyPOV object that was activated and return void.

****Set optional callback function to be called when a TeensyPovDisplay object is updated.****
````
void setUpdateCallback(void (*ptr)(TeensyPovDisplay *))
````
**Arguments:**
- **void (\*ptr)(TeensyPOV \*)** - Pointer to the function to be called. The function must take as an argument a pointer to the TeensyPOV object that was updated and return void.

****Set optional callback function to be called when a TeensyPovDisplay object expires.****
````
void setExpireCallback(void (*ptr)(TeensyPovDisplay *))
````
**Arguments:**
- **void (\*ptr)(TeensyPOV \*)** - Pointer to the function to be called. The function must take as an argument a pointer to the TeensyPOV object that expired and return void.

#### Enums and Structures Defined by TeensyPOV Class:
****Identifies text position at top or bottom of display.****
````
enum TextPosition {TOP, BOTTOM};
````
****Specification for character string to be displayed.****
````
struct DisplayStringSpec {
	const char *characters;
	TextPosition position;
	uint8_t topRow;
	uint8_t textColor;
	uint8_t backgroundColor;
	bool invert;
};
````
- **const char \*characters** - Pointer to null-terminated char array.
- **TextPosition position** - Position where text will be displayed.
- **uint8_t topRow** - LED position (from center) for top row of characters. Minimum value is 6.
- **uint8_t textColors** - Color of text expressed as index into current Palette.
- **uint8_t backgroundColor** - Background color for text expressed as index into current Palette.
- **bool invert** - Flip text on bottom so it appears right-side-up.

****Specification for bit map image to be displayed.****
````
struct LedArrayStruct {
	const uint32_t *array;
	const uint32_t *colors;
	uint32_t numColorBits;
	uint8_t logNumSegments;
	uint32_t columns;
	uint32_t tdcDisplaySegment;
};
````
- **const uint32_t \*array** - Pointer to 2-dimensional bit map array. See below for specification. The array pointed to must be static or global.
- **const uint32_t \*colors** - Pointer to array defining RGB color pallet. Array must have at least 2 ^ numColorBits elements. The array pointed to must be static or global. 
- **uint32_t numColorBits** - Number of color bits. Use static constants defined by class TeensyPOV.
- **uint8_t logNumSegments** - Log (base 2) of number of TDC segments. Use static constants defined by class TeensyPOV.
- **uint32_t columns** - Number of columns in the array.
- **uint32_t tdcDisplaySegment** - Segment number to display when rotating blade hits Top Dead Center.
- 
****Bit Map Array****

The number of Rows is equal to the number of segments in the display. The number of Columns depends on how many color bits are used to represent each LED. For example, with 36 LEDs, Two Bits / LED, and 256 Segments:

![](https://github.com/gfvalvo/TeensyPOV/blob/master/Images/BitMap.jpg)

## POV Hardware:
### Block Diagram:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/BlockDiagram.jpg)
### PWB Schematic:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/Schematic.jpg)
### PWB Layout:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/PWB.jpg)
### Populated PWB:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/20180506_165926.jpg)
### Blank Blade with Hub Mounted:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/20180506_194420.jpg)
### Backside Components:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/20180509_212934.jpg)
### Hall Sensor:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/20180509_213005.jpg)
### LEDs Mounted:
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/20180509_213235.jpg)
### Start with this Fan
[$20 From Target](https://www.target.com/p/holmes-174-20-3-speed-box-fan-black-hbf2001d-b/-/A-15024768)
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/Fan.jpg)
### Reverse these Rails as Shown to Push Motor Further out the Back.
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/20180509_213136.jpg)
### Reverse these Brackets as Shown to Push Motor Further out the Back.
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/20180509_213205.jpg)
### Hall device senses magnet and signal processor that LEDs are in the Top Dead Center position.
![](https://github.com/gfvalvo/TeensyPOV/blob/master/Hardware/20180514_064618.jpg)

# CREDITS
I shamelessly appropriated the Dot Matrix characters that Arduino Forum user @Jiggy-Ninja developed for his POV project posted here:
[Dram Queen](https://forum.arduino.cc/index.php?topic=493075.0)





