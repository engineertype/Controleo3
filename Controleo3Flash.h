// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// Flash controller for W25Q80BV

#ifndef CONTROLEO3FLASH_H_
#define CONTROLEO3FLASH_H_

#include "Arduino.h"
#include "bits.h"

// SCK is PA13
#define FLASH_CLK_ACTIVE      (*portAOut |= SETBIT13)
#define FLASH_CLK_IDLE        (*portAOut &= CLEARBIT13)

// CS is PA14 (D2)
#define FLASH_CS_IDLE         (*portAOut |= SETBIT14)
#define FLASH_CS_ACTIVE       (*portAOut &= CLEARBIT14)

// MOSI is PA16 (D11)
#define FLASH_MOSI_ACTIVE     (*portAOut |= SETBIT16)
#define FLASH_MOSI_IDLE     	(*portAOut &= CLEARBIT16)

// MISO is PA17 (D13)
#define FLASH_MISO_ACTIVE     (*portAOut |= SETBIT17)
#define FLASH_MISO_IDLE       (*portAOut &= CLEARBIT17)
#define FLASH_MISO_HIGH       (*portAIn & SETBIT17)

// WP is PA18 (D10)
#define FLASH_WP_ACTIVE       (*portAOut |= SETBIT18)
#define FLASH_WP_IDLE			    (*portAOut &= CLEARBIT18)

// HOLD is PA19 (D12)
#define FLASH_HOLD_ACTIVE     (*portAOut |= SETBIT19)
#define FLASH_HOLD_IDLE       (*portAOut &= CLEARBIT19)

#define FLASH_PULSE_CLK       { FLASH_CLK_IDLE; FLASH_CLK_ACTIVE; }


class Controleo3Flash {
    public:
    	Controleo3Flash(void);

		  void begin();
      bool verifyFlashIC();
      void waitUntilNotBusy(uint16_t timeMillis);
      void protectFlash(uint8_t flashArea, bool writeToFlash);
      void eraseFlash();
      void startRead(uint16_t pageNumber, uint16_t bytesToRead, uint8_t *dest);
      void continueRead(uint16_t bytesToRead, uint8_t *dest);
      void endRead();
      void write(uint16_t pageNumber, uint16_t bytesToWrite, uint8_t *src);
      void slowRead(uint16_t pageNumber, uint16_t bytesToRead, uint8_t *dest);
      void slowWrite(uint16_t pageNumber, uint16_t bytesToWrite, uint8_t *src);
      void dumpStatusRegisters();
      uint32_t readUniqueID();
      void factoryReset();
      void erasePrefsBlock(uint8_t block);
      void eraseProfileBlock(uint16_t block);
      void allowWritingToPrefs(boolean allow);
      uint16_t getBitmapPage(uint16_t bitmapNumber, uint16_t bitmapWidth, uint16_t bitmapHeight);
      uint16_t getBitmapInfo(uint16_t bitmapNumber, uint16_t *bitmapWidth, uint16_t *bitmapHeight);

private:
  		volatile uint32_t *portAOut, *portAIn, *portAMode;
      void setPinIOMode(uint8_t mode);
      void write8(uint8_t data);
      uint8_t read8();
};

#endif // CONTROLEO3FLASH_H_