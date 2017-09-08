// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// SD card controller
// Based on Arduino's SD library


#include <Arduino.h>
#include "Sd2Card.h"

#define DEBUG_PRINT(x)          SerialUSB.println(F(x))


// At full speed, a data bit is clocked out every 800ns.  So 1.25MHz, well below the data rate
// supported by even the oldest SD cards.  No delays are necessary.  Thank-you oscilloscope!
// And thank-you 32MB SD card from 2001!

// SPI receive
uint8_t Sd2Card::spiRec(void)
{
    uint8_t data = 0;

    // Set the output pin high
    MOSI_ACTIVE;

    for (uint8_t i = 0; i < 8; i++) {
        CLK_ACTIVE;

        data <<= 1;

        if (MISO_HIGH)
            data++;

        CLK_IDLE;
    }
    return data;
}


// SPI send
void Sd2Card::spiSend(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        CLK_IDLE;
        if (data & 0x80)
          MOSI_ACTIVE;
        else
          MOSI_IDLE;
        data <<= 1;
        CLK_ACTIVE;
    }
    CLK_IDLE;
}


// Send command and return error code.  Return zero for OK
uint8_t Sd2Card::cardCommand(uint8_t cmd, uint32_t arg)
{
    // Select card
    CS_ACTIVE;

    // Wait up to 300ms if busy
    waitNotBusy(300);

    // Send command
    spiSend(cmd | 0x40);

    // Send argument
    for (int8_t s = 24; s >= 0; s -= 8)
        spiSend(arg >> s);

    // Send CRC
    spiSend(cmd == CMD0_GO_IDLE_STATE? 0X95: cmd == CMD8_SEND_IF_COND? 0X87: 0xFF);

    // Wait for response
    for (uint8_t i = 0; ((status_ = spiRec()) & 0X80) && i != 0XFF; i++)
        ;
    return status_;
}


// Determine the size of an SD flash memory card.
// Returns the number of 512 byte data blocks in the card or zero if an error occurs.
uint32_t Sd2Card::cardSize(void)
{
    csd_t csd;
    uint32_t blocks = 0;

    // Read the Card Specific Data
    if (!readCSD(&csd))
        return 0;

    // Calculate the size based on the version
    if (csd.v1.csd_ver == 0) {
        SerialUSB.println("Card is version 0");
        uint8_t read_bl_len = csd.v1.read_bl_len;
        uint16_t c_size = (csd.v1.c_size_high << 10) | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low;
        uint8_t c_size_mult = (csd.v1.c_size_mult_high << 1) | csd.v1.c_size_mult_low;
        blocks = (uint32_t)(c_size + 1) << (c_size_mult + read_bl_len - 7);
    }
    else if (csd.v1.csd_ver == 1) {
        SerialUSB.println("Card is version 1");
        uint32_t c_size = ((uint32_t)csd.v2.c_size_high << 16) | (csd.v2.c_size_mid << 8) | csd.v2.c_size_low;
        blocks = (c_size + 1) << 10;
    } else {
        DEBUG_PRINT("Sd2Card::cardSize - Unknown card version");
    }
    return blocks;
}


// Erase a range of blocks.
// This function requests the SD card to do a flash erase for a range of blocks.  The data on the card after an
// erase operation is either 0 or 1, depends on the card vendor.  The card must support single block erase.
uint8_t Sd2Card::erase(uint32_t firstBlock, uint32_t lastBlock) {
    boolean retVal = false;

    // See if single block erase is supported
    if (!eraseSingleBlockEnable()) {
        DEBUG_PRINT("Sd2Card::erase - Single block erase not supported");
        goto done;
    }
    if (type_ != SD_CARD_TYPE_SDHC) {
        firstBlock <<= 9;
        lastBlock <<= 9;
    }
    // Send the erase command
    if (cardCommand(CMD32_ERASE_WR_BLK_START, firstBlock) || cardCommand(CMD33_ERASE_WR_BLK_END, lastBlock) || cardCommand(CMD38_ERASE, 0)) {
        DEBUG_PRINT("Sd2Card::erase - Erase error");
        goto done;
    }
    // Wait for the card to finish the erase
    if (!waitNotBusy(SD_ERASE_TIMEOUT)) {
        DEBUG_PRINT("Sd2Card::erase - Timeout");
        goto done;
    }
    retVal = true;

done:
    CS_IDLE;
    return retVal;
}


// Determine if card supports single block erase.
uint8_t Sd2Card::eraseSingleBlockEnable(void)
{
  csd_t csd;
  return readCSD(&csd) ? csd.v1.erase_blk_en : 0;
}


// Initialize a SD flash memory card.
uint8_t Sd2Card::init()
{
    boolean retVal = false;
    type_ = 0;

    // Save the port addresses (https://github.com/arduino/ArduinoCore-samd/blob/master/cores/arduino/wiring_digital.c)
    portAOut   = portOutputRegister(digitalPinToPort(2));
    portAIn    = portInputRegister(digitalPinToPort(2));
    portAMode  = portModeRegister(digitalPinToPort(2));

    // Set bits to correct I/O
    *portAMode |= (SETBIT05 + SETBIT07 + SETBIT06); // Outputs
    pinMode(PIN_A3, INPUT_PULLUP);  // Input for MISO

    // 16-bit init start time allows over a minute
    uint16_t t0 = (uint16_t) millis();
    uint32_t arg;

    // Set I/O pin modes
    CS_IDLE;

    // Must supply minimum of 74 clock cycles with CS high.
    for (uint8_t i = 0; i < 10; i++)
        spiSend(0XFF);

    CS_ACTIVE;

    // Go idle in SPI mode
    while ((status_ = cardCommand(CMD0_GO_IDLE_STATE, 0)) != R1_IDLE_STATE) {
        if (((uint16_t)(millis() - t0)) > SD_INIT_TIMEOUT) {
            DEBUG_PRINT("Sd2Card::init - Error going idle");
            goto done;
        }
    }

    // Check SD version
    if ((cardCommand(CMD8_SEND_IF_COND, 0x1AA) & R1_ILLEGAL_COMMAND))
        type_ = SD_CARD_TYPE_SD1;
    else {
        // Only need last byte of R7 response
        for (uint8_t i = 0; i < 4; i++)
            status_ = spiRec();
        if (status_ != 0XAA) {
            DEBUG_PRINT("Sd2Card::init - Error getting interface condition");
            goto done;
        }
        type_ = SD_CARD_TYPE_SD2;
    }

    // Initialize card and send host supports SDHC if SD2
    arg = type_ == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;

    while ((status_ = cardAcmd(ACMD41_SD_SEND_OP_COMD, arg)) != R1_READY_STATE) {
        // Check for timeout
        if (((uint16_t)(millis() - t0)) > SD_INIT_TIMEOUT) {
            DEBUG_PRINT("Sd2Card::init - ACMD41 timeout");
            goto next;
        }
    }
next:
    // If SD2 read the OCR register to check for SDHC card
    if (type_ == SD_CARD_TYPE_SD2) {
        if (cardCommand(CMD58_READ_OCR, 0)) {
            DEBUG_PRINT("Sd2Card::init - Read OCR error");
            goto done;
        }
        if ((spiRec() & 0XC0) == 0XC0)
            type_ = SD_CARD_TYPE_SDHC;
        // Discard rest of ocr - contains allowed voltage range
        for (uint8_t i = 0; i < 3; i++)
            spiRec();
    }
    retVal = true;

done:
    CS_IDLE;
    return retVal;
}


// Read a 512 byte block from an SD card device.
uint8_t Sd2Card::readBlock(uint32_t block, uint8_t* dst)
{
  return readData(block, 0, 512, dst);
}


// Read part of a 512 byte block from an SD card.
uint8_t Sd2Card::readData(uint32_t block, uint16_t offset, uint16_t count, uint8_t* dst)
{
    boolean retVal = false;
    uint16_t offset_;

    if (count == 0)
        return true;
    if ((count + offset) > 512)
        return false;

    // Use address if not SDHC card
    if (type_ != SD_CARD_TYPE_SDHC)
        block <<= 9;
    if (cardCommand(CMD17_READ_BLOCK, block)) {
        DEBUG_PRINT("Sd2Card::readData - read error");
        goto done;
    }
    if (!waitStartBlock())
        goto done;

    // Skip data before offset
    for (offset_ = 0; offset_ < offset; offset_++)
        spiRec();

    // Transfer data
    for (uint16_t i = 0; i < count; i++)
        dst[i] = spiRec();

    // Read the rest of the block and the CRC
    offset_ += count;
    while (offset_++ < 514)
        spiRec();

    retVal = true;

done:
    CS_IDLE;
    return retVal;
}


// Read CID or CSR register */
uint8_t Sd2Card::readRegister(uint8_t cmd, void* buf)
{
    boolean retVal = false;
    uint8_t* dst = reinterpret_cast<uint8_t*>(buf);
    if (cardCommand(cmd, 0)) {
        DEBUG_PRINT("Sd2Card::readRegister - Error reading register");
        goto done;
    }
    if (!waitStartBlock())
        goto done;
    // Transfer data
    for (uint16_t i = 0; i < 16; i++)
        dst[i] = spiRec();
    spiRec();  // Get crc bytes
    spiRec();
    retVal = true;

done:
    CS_IDLE;
    return retVal;
}

// Wait for card to go not busy
uint8_t Sd2Card::waitNotBusy(uint16_t timeoutMillis)
{
  uint16_t t0 = millis();
  do {
    if (spiRec() == 0XFF) return true;
  } while (((uint16_t)millis() - t0) < timeoutMillis);
  return false;
}


// Wait for start block token
uint8_t Sd2Card::waitStartBlock(void) {
    boolean retVal = false;
    uint16_t t0 = millis();
    while ((status_ = spiRec()) == 0XFF) {
        if (((uint16_t)millis() - t0) > SD_READ_TIMEOUT) {
            DEBUG_PRINT("Sd2Card::waitStartBlock - read timeout");
            goto done;
        }
    }
    if (status_ != DATA_START_BLOCK) {
        DEBUG_PRINT("Sd2Card::waitStartBlock - read error");
        goto done;
    }
    // Keep CS low
    return true;

done:
    CS_IDLE;
    return retVal;
}


// Writes a 512 byte block to an SD card.
uint8_t Sd2Card::writeBlock(uint32_t blockNumber, const uint8_t* src)
{
//    SerialUSB.print("Sd2Card::writeBlock - writing block ");
//    SerialUSB.println(blockNumber);
    boolean retVal = false;
    // Don't allow write to first block
    if (blockNumber == 0) {
        DEBUG_PRINT("Sd2Card::writeBlock - Can't write to block 0");
        goto done;
    }

    // Use address if not SDHC card
    if (type() != SD_CARD_TYPE_SDHC)
        blockNumber <<= 9;

    if (cardCommand(CMD24_WRITE_BLOCK, blockNumber)) {
        DEBUG_PRINT("Sd2Card::writeBlock - Can't write block start");
        goto done;
    }

    if (!writeData(DATA_START_BLOCK, src))
        goto done;

    // Wait for flash programming to complete
    if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
        DEBUG_PRINT("Sd2Card::writeBlock - Write timeout");
        goto done;
    }

    // Response is R2 so get and check two bytes for nonzero
    if (cardCommand(CMD13_SEND_STATUS, 0) || spiRec()) {
        DEBUG_PRINT("Sd2Card::writeBlock - Write error");
        goto done;
    }
    retVal = true;

done:
    CS_IDLE;
    return retVal;
}


// Write one data block in a multiple block write sequence
uint8_t Sd2Card::writeData(const uint8_t* src)
{
    // Wait for previous write to finish
    if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
        DEBUG_PRINT("Sd2Card::writeData - Write error");
        CS_IDLE;
        return false;
    }
    return writeData(WRITE_MULTIPLE_TOKEN, src);
}


// Send one block of data for write block or write multiple blocks
uint8_t Sd2Card::writeData(uint8_t token, const uint8_t* src)
{
//    SerialUSB.println("Sd2Card::writeData - writing 512 bytes");
    spiSend(token);
    for (uint16_t i = 0; i < 512; i++)
        spiSend(src[i]);
    spiSend(0xff);  // Dummy crc
    spiSend(0xff);  // Dummy crc

    status_ = spiRec();
    if ((status_ & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
        DEBUG_PRINT("Sd2Card::writeData - Write error (token)");
        CS_IDLE;
        return false;
    }
    return true;
}


//  Start a write multiple blocks sequence
uint8_t Sd2Card::writeStart(uint32_t blockNumber, uint32_t eraseCount)
{
    SerialUSB.print("Sd2Card::writeStart - writing block ");
    SerialUSB.print(blockNumber);
    SerialUSB.print(" eraseCount=");
    SerialUSB.println(eraseCount);
    boolean retVal = false;
    // Don't allow write to first block
    if (blockNumber == 0) {
        DEBUG_PRINT("Sd2Card::writeStart - Can't write block 0");
        goto done;
    }
    // Send pre-erase count
    if (cardAcmd(ACMD23_SET_WR_BLK_ERASE_COUNT, eraseCount)) {
        DEBUG_PRINT("Sd2Card::writeStart - Pre-erase error");
        goto done;
    }
    // Use address if not SDHC card
    if (type() != SD_CARD_TYPE_SDHC)
        blockNumber <<= 9;
    if (cardCommand(CMD25_WRITE_MULTIPLE_BLOCK, blockNumber)) {
        DEBUG_PRINT("Sd2Card::writeStart - error");
        goto done;
    }
    // Keep CS low
    return true;

done:
    CS_IDLE;
    return retVal;
}


// End a write multiple blocks sequence
uint8_t Sd2Card::writeStop(void)
{
    SerialUSB.print("Sd2Card::writeStop - writing block ");
    boolean retVal = false;
    if (!waitNotBusy(SD_WRITE_TIMEOUT))
        goto done;
    spiSend(STOP_TRAN_TOKEN);
    if (!waitNotBusy(SD_WRITE_TIMEOUT))
        goto done;
    retVal = true;

done:
    CS_IDLE;
    return retVal;
}
