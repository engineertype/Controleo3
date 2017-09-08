// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// Touch controller for XPT2046
//


// For general software development, TOUCH_DEBUG should be enabled so that
// range checking happens on all parameters ensuring that all drawing takes
// place on the screen.  For release, TOUCH_DEBUG can be disabled to improve
// drawing speed.
#define TOUCH_DEBUG

#include "Controleo3Touch.h"

Controleo3Touch::Controleo3Touch()
{
    // Get the addresses of Port A (D2 is on port A)
    portAOut   = portOutputRegister(digitalPinToPort(2));
    portAIn    = portInputRegister(digitalPinToPort(2));
    portAMode  = portModeRegister(digitalPinToPort(2));

    // Get the addresses of Port B (A2 is on port B)
    portBOut   = portOutputRegister(digitalPinToPort(A2));
    portBIn    = portInputRegister(digitalPinToPort(A2));
    portBMode  = portModeRegister(digitalPinToPort(A2));
}


void Controleo3Touch::begin()
{
    // Set some pins as outputs (CLK, CS, MOSI)
    *portAMode |= (SETBIT08 + SETBIT09 + SETBIT10);

    // Set some pins as inputs (MISO, PEN_IRQ)
    *portAMode &= CLEARBIT11;	// MISO
    *portAOut  &= CLEARBIT11;	// MISO
    pinMode(23, INPUT);			// PEN_IRQ

    // Initialize pins states
    TOUCH_CLK_ACTIVE;
    TOUCH_CS_ACTIVE;
    TOUCH_MOSI_ACTIVE;
}


void Controleo3Touch::calibrate(int16_t tlX,int16_t trX,int16_t blX,int16_t brX,int16_t tlY,int16_t blY,int16_t trY,int16_t brY)
{
    topLeftX = tlX;
    topRightX = trX;
    bottomLeftX = blX;
    bottomRightX = brX;
    topLeftY = tlY;
    bottomLeftY = blY;
    topRightY = trY;
    bottomRightY = brY;
#ifdef TOUCH_DEBUG
    SerialUSB.print("Touch data: tlX=" + String(tlX));
    SerialUSB.print(" trX=" + String(trX));
    SerialUSB.print(" blX=" + String(blX));
    SerialUSB.print(" brX=" + String(brX));
    SerialUSB.print(" tlY=" + String(tlY));
    SerialUSB.print(" blY=" + String(blY));
    SerialUSB.print(" trY=" + String(trY));
    SerialUSB.println(" brY=" + String(brY));
#endif
}


// If the touchscreen is pressed the IRQ pin will be high
bool Controleo3Touch::isPressed(void)
{
    return TOUCH_PEN_IRQ;
}


#define NUM_SAMPLES  	8
#define MAX_DEVIATION   20

// Read touch controller samples.  Returns true if the reading is valid, or false if
// there is no touch, or the touch readings deviate too much (indicating rapid finger movement up/down).
// Eight samples are taken and averaged out to get a more accurate indication of where the user is touching
// on the screen.
bool Controleo3Touch::readRaw(int16_t *x, int16_t *y)
{
    uint16_t xValues[NUM_SAMPLES];
    uint16_t yValues[NUM_SAMPLES];
    
	// See if there is a touch
    if (!TOUCH_PEN_IRQ)
    	return false;

	TOUCH_CS_IDLE;

    for (int count = 0; count < NUM_SAMPLES; count++) {
        // Read X value
        write8(0x90);
        TOUCH_MOSI_IDLE;
        TOUCH_PULSE_CLK;
        xValues[count] = read12();
        // Read Y value
        write8(0xD0);
        TOUCH_MOSI_IDLE;
        TOUCH_PULSE_CLK;
        yValues[count] = read12();
    }

	TOUCH_CS_ACTIVE;

    // A high deviation indicates the finger is moving onto or away from the screen
    // Reject if X values differ by more than DEVIATION
    if (calcDeviation(xValues, NUM_SAMPLES, x) > MAX_DEVIATION)
      return false;

    // Reject if Y values differ by more than DEVIATION
    if (calcDeviation(yValues, NUM_SAMPLES, y) > MAX_DEVIATION)
      return false;

#ifdef TOUCH_DEBUG
    SerialUSB.print("X = ");
    SerialUSB.print(*x);
    SerialUSB.print("   Y = ");
    SerialUSB.println(*y);
#endif
    return true;
}


// Gets raw touch data and then maps it to the LCD coordinates
bool Controleo3Touch::read(int16_t *x, int16_t *y)
{
    // Read the raw touch values
    if (readRaw(x, y) == false)
      return false;

    // Get the approximate Y value
    int16_t yy = map(*y, topLeftY, bottomRightY, 0, LCD_MAX_Y);

    // Get a weighted X value and constrain it to the screen
    *x = ((map(*x, topLeftX, topRightX, 0, LCD_MAX_X) * yy) + (map(*x, bottomLeftX, bottomRightX, 0, LCD_MAX_X) * (LCD_MAX_Y - yy))) / LCD_HEIGHT;
    *x = constrain(*x, 0, LCD_MAX_X);

    // Get a weighted Y value
    *y = ((map(*y, topLeftY, bottomLeftY, 0, LCD_MAX_Y) * *x) + (map(*y, topRightY, bottomRightY, 0, LCD_MAX_Y) * (LCD_MAX_X - *x))) / LCD_WIDTH;
    *y = constrain(*y, 0, LCD_MAX_Y);
    return true;
}


// Find the lowest and highest number in the array, and return the difference.  Also calculate
// the average of the numbers in the array.
uint16_t Controleo3Touch::calcDeviation(uint16_t *array, uint8_t num, int16_t *average) {
  uint16_t deviation;
  int32_t sum, min, max;

  sum = min = max = array[0];
  for (int count = 1; count < num; count++) {
    if (max < array[count])
      max = array[count];
    if (min > array[count])
      min = array[count];
    sum += array[count];
  }

  deviation = max - min;
  *average  = sum / num;
  return deviation;
}


// Write 8 bits of data to the IC
void Controleo3Touch::write8(byte data)
{
	for (byte count=0; count<8; count++)
	{
		if (data & 0x80)
			TOUCH_MOSI_ACTIVE;
		else
			TOUCH_MOSI_IDLE;
		data = data << 1;
		TOUCH_PULSE_CLK;
	}
}


// Read the touch value
uint16_t Controleo3Touch::read12()
{
	uint16_t data = 0;

	for (byte count=0; count<12; count++)
	{
		data <<= 1;
		TOUCH_PULSE_CLK;
		if (TOUCH_MISO_HIGH)
			data++;
	}
	return data;
}
