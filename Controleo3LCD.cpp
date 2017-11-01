// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// LCD controller for ILI9488 controller


// For general software development, LCD_DEBUG should be enabled so that
// range checking happens on all parameters ensuring that all drawing takes
// place on the screen.  For release, LCD_DEBUG can be disabled to improve
// drawing speed.
//#define LCD_DEBUG

#include "Controleo3LCD.h"


// Constructor for the TFT display
Controleo3LCD::Controleo3LCD(void)
{
    // Get the addresses of Port B
    portBOut   = portOutputRegister(digitalPinToPort(A2));
    portBMode  = portModeRegister(digitalPinToPort(A2));
    portBIn    = portInputRegister(digitalPinToPort(A2));
    flood8Reg  = ((uint8_t *) portBOut) + 1;
    bitmapReg  = ((uint16_t *) portBOut);
}


void Controleo3LCD::begin()
{
    // Set RD, WR, CD, CS and data pins to be outputs
    *portBMode |= (SETBIT12 + SETBIT13 + SETBIT14 + SETBIT15 + SETBIT16 + 0xFF);

    // Pull RD high so writes work
    LCD_RD_ACTIVE;

	// Do a hard reset
    reset(HARD_RESET);

	LCD_CS_ACTIVE;
    writeRegister8(ILI9488_PIXELFORMAT, 0x55);
    writeRegister8(ILI9488_MADCTL, ILI9488_MADCTL_MV | ILI9488_MADCTL_BGR);
    write8Command(ILI9488_SLEEPOUT);
    LCD_CS_IDLE;

	// Wait for things to settle before returning
    delay(15);
}


// Set the I/O mode on the read pin
void Controleo3LCD::readMode(boolean enable)
{
    if (enable) {
        // Set data pins to input mode
        for (int i=0; i<8; i++) {
            PORT->Group[PORTB].PINCFG[i].reg=(uint8_t)(PORT_PINCFG_INEN);
            PORT->Group[PORTB].DIRCLR.reg = (uint32_t)(1<<i);
        }
    }
    else {
        // Set pin to output mode
        for (int i=0; i<8; i++) {
            PORT->Group[PORTB].PINCFG[i].reg&=~(uint8_t)(PORT_PINCFG_INEN);
            PORT->Group[PORTB].DIRSET.reg = (uint32_t)(1<<i);
        }
    }
}


// Reset the TFT Display
// type = HARD_RESET or SOFT_RESET
void Controleo3LCD::reset(uint8_t type)
{
    if (type == HARD_RESET) {
        LCD_RESET_LOW;
        delayMicroseconds(10);
        LCD_RESET_HIGH;
    }
    else {
        write8Command(ILI9488_SOFTRESET);
        LCD_CS_IDLE;
    }

    // In both cases the display needs 5ms to recover from the reset
    delay(5);
}


// Get the LCD's version
uint32_t Controleo3LCD::getLCDVersion()
{
    uint32_t version = 0;

    write8Command(ILI9488_READ_DISPLAY_INFO);
    LCD_WR_ACTIVE;
    readMode(true);
    // Toggle read bit before starting to read data
    LCD_RD_IDLE;

    for (uint8_t i = 0; i < 3; i++) {
        version = version << 8;
        version += read8Data();
    }
    LCD_CS_IDLE;
    readMode(false);
    // Pull RD high so writes work
    LCD_RD_ACTIVE;
    LCD_WR_IDLE;
    return version;
}


// Set the addressable range of the window
void Controleo3LCD::setAddrWindow(int x1, int y1, int x2, int y2)
{
#ifdef LCD_DEBUG
	checkRange(x1, 0, LCD_MAX_X, "setAddrWindow:x1");
	checkRange(x2, 0, LCD_MAX_X, "setAddrWindow:x2");
	checkRange(y1, 0, LCD_MAX_Y, "setAddrWindow:y1");
	checkRange(y2, 0, LCD_MAX_Y, "setAddrWindow:y2");
#endif
    writeRegister16x2(ILI9488_COLADDRSET, x1, x2);
    writeRegister16x2(ILI9488_PAGEADDRSET, y1, y2);
}


// Fills the window (set using setAddrWindow) with pixels of the specified color
// This routine is (fairly) heavily optimized for performance.
void Controleo3LCD::flood(uint16_t color, uint32_t len)
{
    uint8_t high = highByte(color), low = lowByte(color);

    write8Command(ILI9488_MEMORYWRITE);

    // Optimize the case where high == low
    if (high == low) {
        // Write the first pixel to set the color
        write8Data(high);
        LCD_WR_IDLE;
        LCD_WR_ACTIVE;
        len--;

        // Get the current value of the register used for the write bit
        // This method is dangerous because we'll be ignoring changes in the other
        // bits for the duration of these writes.  However, the affected pins are
        // Relays 4 to 6, Touch IRQ and some LCD pins so there is no downside.
        uint8_t writeIdle = *flood8Reg & 0xDF;
        uint8_t writeActive = writeIdle | 0x20;

        #define WR_STROBE       {*flood8Reg = writeIdle; *flood8Reg = writeActive;}

        uint32_t setsOf8 = len >> 3;
        len &= 0x7;

        // Do 16 pixels at a time
        // Time to clear screen:
        // 1 at a time = 64ms
        // 8 at a time = 54ms
        // 16 at a time = 55ms (yes, longer)
        while (setsOf8--) {
            WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;
            WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;WR_STROBE;
        }
        while (len--) {
            WR_STROBE;
            WR_STROBE;
        }
        return;
    }

    while (len--) {
        write8Data(high);
        write8Data(low);
    }
}


// Draw a horizontal line
void Controleo3LCD::drawFastHLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
#ifdef LCD_DEBUG
	checkRange(x, 0, LCD_MAX_X, "drawFastHLine:x");
	checkRange(y, 0, LCD_MAX_Y, "drawFastHLine:y");
	checkRange(length, 1, LCD_MAX_X - x, "drawFastHLine:length");
#endif

    setAddrWindow(x, y, x + length - 1, y);
    flood(color, length);
    LCD_CS_IDLE;
}


// Draw a vertical line
void Controleo3LCD::drawFastVLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
#ifdef LCD_DEBUG
	checkRange(x, 0, LCD_MAX_X, "drawFastVLine:x");
	checkRange(y, 0, LCD_MAX_Y, "drawFastVLine:y");
	checkRange(length, 1, LCD_MAX_Y - y, "drawFastVLine:length");
#endif

    setAddrWindow(x, y, x, y + length - 1);
    flood(color, length);
    LCD_CS_IDLE;
}


// Draw a rectangle with the given color
void Controleo3LCD::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
#ifdef LCD_DEBUG
	checkRange(x, 0, LCD_MAX_X, "drawRect:x");
	checkRange(y, 0, LCD_MAX_Y, "drawRect:y");
	checkRange(w, 1, LCD_WIDTH - x, "drawRect:w");
	checkRange(h, 1, LCD_HEIGHT - y, "drawRect:h");
#endif
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y+h-1, w, color);
    if (h >= 3) {
      drawFastVLine(x, y+1, h-2, color);
      drawFastVLine(x+w-1, y+1, h-2, color);
    }
}


// Fill the rectangle with the given color
void Controleo3LCD::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fillcolor)
{
#ifdef LCD_DEBUG
	checkRange(x, 0, LCD_MAX_X, "fillRect:x");
	checkRange(y, 0, LCD_MAX_Y, "fillRect:y");
	checkRange(w, 1, LCD_WIDTH - x, "fillRect:w");
	checkRange(h, 1, LCD_HEIGHT - y, "fillRect:h");
#endif

    setAddrWindow(x, y, x + w - 1, y + h - 1);
    flood(fillcolor, (uint32_t) w * (uint32_t) h);
    LCD_CS_IDLE;
}


// Fill the screen with the given color
void Controleo3LCD::fillScreen(uint16_t color) {
	// The screen takes rotation into account
    setAddrWindow(0, 0, LCD_MAX_X, LCD_MAX_Y);
    flood(color, (uint32_t) LCD_WIDTH * (uint32_t) LCD_HEIGHT);
    LCD_CS_IDLE;
}


// Draw a pixel at the specified location
void Controleo3LCD::drawPixel(int16_t x, int16_t y, uint16_t color) {
#ifdef LCD_DEBUG
	checkRange(x, 0, LCD_MAX_X, "drawPixel:x");
	checkRange(y, 0, LCD_MAX_Y, "drawPixel:y");
#endif

    setAddrWindow(x, y, x, y);
    writeRegister16(ILI9488_MEMORYWRITE,color);
    LCD_CS_IDLE;
}


// Start drawing a bitmap
void Controleo3LCD::startBitmap(int16_t x, int16_t y, int16_t w, int16_t h)
{
#ifdef LCD_DEBUG
	checkRange(x, 0, LCD_MAX_X, "fillRect:x");
	checkRange(y, 0, LCD_MAX_Y, "fillRect:y");
	checkRange(w, 1, LCD_WIDTH - x, "fillRect:w");
	checkRange(h, 1, LCD_HEIGHT - y, "fillRect:h");
#endif

    setAddrWindow(x, y, x + w - 1, y + h - 1);
    write8Command(ILI9488_MEMORYWRITE);

    bitmapRegValue = *bitmapReg & 0xDF00;    // Clear the write bit
}


// Draw part or all of a bitmap.  The bitmaps must be 16-bit color.
// This function can be called over and over again until the whole
// bitmap has been rendered to the screen.
void Controleo3LCD::drawBitmap(uint16_t *data, uint32_t len)
{
#define write8DataBitmap(d)  *bitmapReg = (bitmapRegValue + d); LCD_WR_ACTIVE;
	while(len--) {
    	write8DataBitmap(highByte(*data));
    	write8DataBitmap(lowByte(*data));
        data++;
    }
}


// End the drawing of the bitmap
void Controleo3LCD::endBitmap()
{
    LCD_CS_IDLE;
}


// Allow users to write directly to LCD registers.  See ILI9488.h
void Controleo3LCD::pokeRegister(uint8_t reg)
{
    write8Command(reg);
    LCD_CS_IDLE;
}


// Allow users to write directly to LCD registers.  See ILI9488.h
void Controleo3LCD::setRegister8(uint8_t a, uint8_t d)
{
    writeRegister8(a,d);
    LCD_CS_IDLE;
}


void Controleo3LCD::startReadBitmap(int16_t x, int16_t y, int16_t w, int16_t h)
{
#ifdef LCD_DEBUG
	checkRange(x, 0, LCD_MAX_X, "fillRect:x");
	checkRange(y, 0, LCD_MAX_Y, "fillRect:y");
	checkRange(w, 1, LCD_WIDTH - x, "fillRect:w");
	checkRange(h, 1, LCD_HEIGHT - y, "fillRect:h");
#endif

    setAddrWindow(x, y, x + w - 1, y + h - 1);
    write8Command(ILI9488_MEMORYREAD);
    LCD_WR_ACTIVE;
    readMode(true);
    // Toggle read bit before starting to read data
    LCD_RD_IDLE;
}


// Read a bitmap (used for screenshots)
void Controleo3LCD::readBitmapRGB565(uint16_t *data, uint32_t len)
{
	while(len--) {
        // Red
    	*data = (read8Data() & 0xF8) << 8;
        // Green
        *data += (read8Data() & 0xFC) << 3;
        // Blue
        *data += (read8Data() & 0xF8) >> 3;

        data++;
    }
}


// Draw part or all of a bitmap.  The bitmaps must be 16-bit color.
// This function can be called over and over again until the whole
// bitmap has been rendered to the screen.
void Controleo3LCD::readBitmap24bit(uint8_t *data, uint32_t len)
{
	while(len--) {
        *(data+2) = read8Data();
        *(data+1) = read8Data();
        *(data+0) = read8Data();
        data+= 3;
    }
}


// End the drawing of the bitmap
void Controleo3LCD::endReadBitmap()
{
    LCD_CS_IDLE;
    readMode(false);
    // Pull RD high so writes work
    LCD_RD_ACTIVE;
    LCD_WR_IDLE;
}


// Read 8-bits of data from the screen
uint8_t Controleo3LCD::read8Data()
{
    // Toggle the read bit
    LCD_RD_ACTIVE;
    LCD_RD_IDLE;
    return *portBIn;
}


// Convert a 24-bit RGB value to 16-bit (RGB565)
uint16_t Controleo3LCD::convertTo16Bit(uint32_t bit24)
{
    uint16_t val = 0;
    val = (bit24 & 0x00F80000) >> 8;
    val |= (bit24 & 0x0000FC00) >> 5;
    val |= (bit24 & 0x000000F8) >> 3;
    return val;
}


#ifdef LCD_DEBUG
// Make sure the given value is in the range specified.
void Controleo3LCD::checkRange(int val, int low, int high, char *msg)
{
	if (val < low) {
    	SerialUSB.print(msg);
        SerialUSB.print(": value ");
        SerialUSB.print(val);
        SerialUSB.print(" is smaller than ");
        SerialUSB.println(low);
    }
	if (val > high) {
    	SerialUSB.print(msg);
        SerialUSB.print(": value ");
        SerialUSB.print(val);
        SerialUSB.print(" is larger than ");
        SerialUSB.println(high);
    }
}
#endif

