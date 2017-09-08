// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// LCD controller for ILI9488 controller

#ifndef CONTROLEO3LCD_H_
#define CONTROLEO3LCD_H_

#include "Arduino.h"
#include "bits.h"
#include "ILI9488.h"

#define LCD_WIDTH  		480
#define LCD_HEIGHT 		320
#define LCD_MAX_X		479
#define LCD_MAX_Y		319


// RD is PB12
#define LCD_RD_ACTIVE       (*portBOut |= SETBIT12)
#define LCD_RD_IDLE			    (*portBOut &= CLEARBIT12)

// WR is PB13
#define LCD_WR_ACTIVE       (*portBOut |= SETBIT13)
#define LCD_WR_IDLE			    (*portBOut &= CLEARBIT13)

// CD is PB14
#define LCD_CD_DATA         (*portBOut |= SETBIT14)
#define LCD_CD_COMMAND      (*portBOut &= CLEARBIT14)

// CS is PB15
#define LCD_CS_IDLE         (*portBOut |= SETBIT15)
#define LCD_CS_ACTIVE       (*portBOut &= CLEARBIT15)

// RESET is PB16
#define LCD_RESET_HIGH      (*portBOut |= SETBIT16)
#define LCD_RESET_LOW     	(*portBOut &= CLEARBIT16)


// Command terminator: set WR and CD
#define LCD_END_COMMAND     (*portBOut |= (SETBIT13 + SETBIT14))


#define write8Command(d)            { *portBOut &= 0xFFFF1F00; *portBOut |= (d); LCD_END_COMMAND;}
#define write8Data(d)               { *portBOut &= 0xFFFFDF00; *portBOut |= (d); LCD_WR_ACTIVE; }

#define writeRegister8(a,d)		    { write8Command(a); write8Data(d);}
#define writeRegister16(a,d)	    { write8Command(a); write8Data(highByte(d)); write8Data(lowByte(d));}
#define writeRegister16x2(a,d1,d2)	{ write8Command(a); write8Data(highByte(d1)); write8Data(lowByte(d1)); \
                                    write8Data(highByte(d2)); write8Data(lowByte(d2));}


#define HARD_RESET              0
#define SOFT_RESET              1

// Some common colors
#define BLACK                   0x0000      //   0,   0,   0
#define NAVY                    0x000F      //   0,   0, 128
#define DARK_GREEN              0x03E0      //   0, 128,   0
#define DARK_CYAN               0x03EF      //   0, 128, 128
#define MAROON                  0x7800      // 128,   0,   0
#define PURPLE                  0x780F      // 128,   0, 128
#define OLIVE                   0x7BE0      // 128, 128,   0
#define LIGHT_GREY              0xC618      // 192, 192, 192
#define DARK_GREY               0x7BEF      // 128, 128, 128
#define BLUE                    0x001F      //   0,   0, 255
#define GREEN                   0x07E0      //   0, 255,   0
#define CYAN                    0x07FF      //   0, 255, 255
#define RED                     0xF800      // 255,   0,   0
#define MAGENTA                 0xF81F      // 255,   0, 255
#define YELLOW                  0xFFE0      // 255, 255,   0
#define WHITE                   0xFFFF      // 255, 255, 255
#define ORANGE                  0xFD20      // 255, 165,   0
#define GREEN_YELLOW            0xAFE5      // 173, 255,  47
#define PINK                    0xF81F      // 255, 192, 203


class Controleo3LCD
{
	public:
  	Controleo3LCD(void);

  	void begin();
  	void reset(uint8_t type);
  	void drawPixel(int16_t x, int16_t y, uint16_t color);
  	void drawFastHLine(int16_t x0, int16_t y0, int16_t w, uint16_t color);
  	void drawFastVLine(int16_t x0, int16_t y0, int16_t h, uint16_t color);
  	void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
  	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
  	void fillScreen(uint16_t color);

  	void startBitmap(int16_t x, int16_t y, int16_t w, int16_t h);
  	void endBitmap();
  	void drawBitmap(uint16_t *data, uint32_t len);

    void startReadBitmap(int16_t x, int16_t y, int16_t w, int16_t h);
    void readBitmapRGB565(uint16_t *data, uint32_t len);
    void readBitmap24bit(uint8_t *data, uint32_t len);
    void endReadBitmap();

  	void pokeRegister(uint8_t reg);
  	void setRegister8(uint8_t a, uint8_t d);
    uint32_t getLCDVersion();
    uint16_t convertTo16Bit(uint32_t val);


	private:
		void setAddrWindow(int x1, int y1, int x2, int y2);
		void flood(uint16_t color, uint32_t len);
    void readMode(boolean enable);
    uint8_t read8Data();
		volatile uint32_t *portBOut, *portBMode, *portBIn;
    volatile uint8_t *flood8Reg;
    volatile uint16_t *bitmapReg;
    uint16_t bitmapRegValue;
		void checkRange(int val, int low, int high, char *msg);
};


#endif // CONTROLEO3LCD_H_
