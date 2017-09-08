// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// Touch controller for XPT2046


#ifndef CONTROLEO3TOUCH_H_
#define CONTROLEO3TOUCH_H_

#include "Arduino.h"
#include "bits.h"
#include "ILI9488.h"

#define LCD_WIDTH  		480
#define LCD_HEIGHT 		320
#define LCD_MAX_X		479
#define LCD_MAX_Y		319


// CLK is D4 (PA8)
#define TOUCH_CLK_ACTIVE        (*portAOut |= SETBIT08)
#define TOUCH_CLK_IDLE		    (*portAOut &= CLEARBIT08)

// CS is D3 (PA9)
#define TOUCH_CS_ACTIVE         (*portAOut |= SETBIT09)
#define TOUCH_CS_IDLE			(*portAOut &= CLEARBIT09)

// MOSI is D1 (PA10)
#define TOUCH_MOSI_ACTIVE       (*portAOut |= SETBIT10)
#define TOUCH_MOSI_IDLE         (*portAOut &= CLEARBIT10)

// MISO is D0 (PA11)
#define TOUCH_MISO_HIGH  		(*portAIn & SETBIT11)

// PEN_IRQ is on PB10
#define TOUCH_PEN_IRQ           ((*portBIn & SETBIT10) == 0)

#define TOUCH_PULSE_CLK         { TOUCH_CLK_IDLE; TOUCH_CLK_ACTIVE; }


class Controleo3Touch
{
	public:
    	Controleo3Touch(void);

		void begin();
        void calibrate(int16_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t,int16_t);
		bool read(int16_t *x, int16_t *y);
		bool readRaw(int16_t *x, int16_t *y);
        bool isPressed();

    private:
  		volatile uint32_t *portAOut, *portAIn, *portAMode, *portBOut, *portBIn, *portBMode;
        int16_t topLeftX,topRightX,bottomLeftX,bottomRightX,topLeftY,bottomLeftY,topRightY,bottomRightY;
		void write8(byte data);
		word read12();
        uint16_t calcDeviation(uint16_t *array, uint8_t num, int16_t *average);
};

#endif // CONTROLEO3TOUCH_H_