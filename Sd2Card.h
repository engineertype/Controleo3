// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// SD card controller
// Based on Arduino's SD library


#ifndef SD2CARD_H_
#define SD2CARD_H_
#include <Arduino.h>
#include "SdInfo.h"
#include "bits.h"


// SCK is PA13
#define CLK_ACTIVE      (*portAOut |= SETBIT05)
#define CLK_IDLE     	(*portAOut &= CLEARBIT05)

// CS is PA14 (D2)
#define CS_IDLE         (*portAOut |= SETBIT07)
#define CS_ACTIVE       (*portAOut &= CLEARBIT07)

// MOSI is PA16 (D11)
#define MOSI_ACTIVE     (*portAOut |= SETBIT06)
#define MOSI_IDLE     	(*portAOut &= CLEARBIT06)

// MISO is PA17 (D13)
#define MISO_ACTIVE     (*portAOut |= SETBIT04)
#define MISO_IDLE		(*portAOut &= CLEARBIT04)
#define MISO_HIGH  		(*portAIn & SETBIT04)


#define SD_INIT_TIMEOUT                 2000    // Init timeout ms
#define SD_ERASE_TIMEOUT                10000   // Erase timeout ms
#define SD_READ_TIMEOUT                 300     // Read timeout ms
#define SD_WRITE_TIMEOUT                600     // Write time out ms

// SD card types
#define SD_CARD_TYPE_SD1                1       // Standard capacity V1 SD card
#define SD_CARD_TYPE_SD2                2       // Standard capacity V2 SD card
#define SD_CARD_TYPE_SDHC               3       // High Capacity SD card


// Sd2Card class - Raw access to SD and SDHC flash memory cards.
class Sd2Card {
public:
    uint32_t cardSize(void);
    uint8_t erase(uint32_t firstBlock, uint32_t lastBlock);
    uint8_t eraseSingleBlockEnable(void);
    uint8_t init(void);
    uint8_t readBlock(uint32_t block, uint8_t* dst);
    uint8_t readData(uint32_t block, uint16_t offset, uint16_t count, uint8_t* dst);
    // Read a cards CID register. The CID contains card identification information such as Manufacturer ID,
    // Product name, Product serial number and Manufacturing date.
    uint8_t readCID(cid_t* cid) { return readRegister(CMD10_SEND_CID, cid); }
    // Read a cards CSD register. The CSD contains Card-Specific Data that provides information
    // regarding access to the card's contents.
    uint8_t readCSD(csd_t* csd) { return readRegister(CMD9_SEND_CSD, csd); }
    uint8_t type(void)  { return type_; }
    uint8_t writeBlock(uint32_t blockNumber, const uint8_t* src);
    uint8_t writeData(const uint8_t* src);
    uint8_t writeData(uint8_t token, const uint8_t* src);
    uint8_t writeStart(uint32_t blockNumber, uint32_t eraseCount);
    uint8_t writeStop(void);

private:
    volatile uint32_t *portAOut, *portAIn, *portAMode;
    uint8_t status_;
    uint8_t type_;
    uint8_t cardAcmd(uint8_t cmd, uint32_t arg) { cardCommand(CMD55_APP_CMD, 0); return cardCommand(cmd, arg); }
    uint8_t cardCommand(uint8_t cmd, uint32_t arg);
    uint8_t readRegister(uint8_t cmd, void* buf);
    uint8_t waitNotBusy(uint16_t timeoutMillis);
    uint8_t waitStartBlock(void);
    uint8_t spiRec(void);
    void    spiSend(uint8_t data);
};
#endif  // SD2CARD_H_
