// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// Flash controller for W25Q80BV

#include "Controleo3Flash.h"

// Pin IO modes
#define PIN_IO_NORMAL                   0
#define PIN_IO_QUAD_READ                1
#define PIN_IO_QUAD_WRITE               2

// Status register bits
#define STATUS_BUSY                     0x01
#define STATUS_WRITE_ENABLE             0x02
#define STATUS_BP0                      0x04
#define STATUS_BP1                      0x08
#define STATUS_BP2                      0x10
#define STATUS_TB                       0x20
#define STATUS_SEC                      0x40
#define STATUS_SRP0                     0x80
#define STATUS_SRP1                     0x01
#define STATUS_QE                       0x02
#define STATUS_LB1                      0x08
#define STATUS_LB2                      0x10
#define STATUS_LB3                      0x20
#define STATUS_CMP                      0x40

// Areas of flash to protect
#define PROTECT_ALL                     0
#define PROTECT_NONE                    1
#define PROTECT_NOT_PREFS               2
#define TEMPORARY_PROTECTION            0   // Settings are changed in RAM, and not written to flash
#define PERMANENT_PROTECTION            1   // Settings are changed in RAM and flash

// Commands
#define CMD_WRITE_STATUS_REGISTER       0x01
#define CMD_WRITE_DISABLE               0x04
#define CMD_READ_STATUS1_REGISTER       0x05
#define CMD_WRITE_ENABLE                0x06
#define CMD_ERASE_SECTOR_4K             0x20
#define CMD_QUAD_INPUT_PAGE_PROGRAM     0x32
#define CMD_READ_STATUS2_REGISTER       0x35
#define CMD_WRITE_SECURITY_REGISTER     0x42
#define CMD_ERASE_SECURITY_REGISTER     0x44
#define CMD_READ_SECURITY_REGISTER      0x48
#define CMD_READ_UNIQUE_ID              0x4B
#define CMD_VOLATILE_STATUS_REGISTER    0x50
#define CMD_ERASE_FLASH                 0x60
#define CMD_OCTAL_WORD_READ_QUAD        0xE3
#define CMD_MANUFACTURER_ID             0x90
#define CMD_JEDEC_ID                    0x9F
#define CMD_ERASE_BLOCK_64K             0xD8

#define SEND_CMD(x)                     {FLASH_CS_ACTIVE; write8(x); FLASH_CS_IDLE; }


// Flash storage organization by pages. Pages are 256 bytes in size. The smallest block
// that can be erased at a time is 16 pages (4K).
// 0 to 511 (128K) - Preferences
//   - 0 to 15 (4K)  = Prefs1 (Location for prefs storage alternates so they won't be lost during write power failure)
//   - 16 to 31 (4K) = Prefs2
//   - 32 to 47 (4K)  = Prefs3
//   - 48 to 63 (4K) = Prefs4
//   - 64 to 511 (28 x 4K) = Profiles
// 512 to 527 (16 pages, 4K) - Bitmap address table
//   - Each bitmap uses 6 bytes
//     2 bytes for bitmap start page
//     2 bytes for bitmap width
//     2 bytes for bitmap height
//   - Entries are stored 42 per page, so maximum 672 bitmaps
// 528 to 4095 (892K) - bitmaps
//   - Bitmaps are saved to page boundaries
//   - Bitmaps are 16-bit 565RGB color

#define FLASH_BITMAP_ADDRESS_TABLE          512
#define FLASH_ADDRESSES_PER_PAGE            42
#define FLASH_FIRST_BITMAP_PAGE             528
#define FLASH_C3_PAGE_SIZE                  256
#define FLASH_MAXIMUM_BITMAPS               672
#define FLASH_ADDRESS_SIZE                  6



Controleo3Flash::Controleo3Flash()
{
    // Get the addresses of Port A (D2 is on port A)
    portAOut   = portOutputRegister(digitalPinToPort(2));
    portAIn    = portInputRegister(digitalPinToPort(2));
    portAMode  = portModeRegister(digitalPinToPort(2));
}


void Controleo3Flash::begin()
{
    // Set the pin IO states
    setPinIOMode(PIN_IO_NORMAL);

    // Default pin states
    FLASH_CS_IDLE;
    FLASH_CLK_ACTIVE;

    // Protect the flash
    protectFlash(PROTECT_ALL, TEMPORARY_PROTECTION);
}


// Sets the pins to be INPUT or OUTPUT, depending on how the IC is accessed. Quad-bit
// mode is used where possible.  This enables faster read and writes because 4-bits of
// data is clocked in at a time, instead of just 1.
void Controleo3Flash::setPinIOMode(uint8_t mode)
{
    switch (mode) {
        case PIN_IO_NORMAL:     // Used for single-bit I/O mode
            *portAMode |= (SETBIT13 + SETBIT14 + SETBIT16 + SETBIT18 + SETBIT19);
            *portAMode &= CLEARBIT17;   // Set MISO as an input
            FLASH_HOLD_ACTIVE;
            break;
        case PIN_IO_QUAD_READ:
            // Used for quad-bit read mode. IO0, IO1, IO2 and IO3 pins are inputs
            *portAMode &= ~(SETBIT16 + SETBIT17 + SETBIT18 + SETBIT19);
            break;
        case PIN_IO_QUAD_WRITE:
            // Used for quad-bit write mode. IO0, IO1, IO2 and IO3 pins are outputs
            *portAMode |= (SETBIT16 + SETBIT17 + SETBIT18 + SETBIT19);
            break;
    }
}


// Verify that the correct Flash IC has been installed, and that communication
// to it works fine.
bool Controleo3Flash::verifyFlashIC()
{
    const char *msg = 0;

    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Verify the JEDEC ID and flash size
    FLASH_CS_ACTIVE;
    write8(CMD_JEDEC_ID);
    if (read8() != 0xEF)
        msg = "Err:verifyFlashIC:JEDEC";
    if (read8() != 0x40)
        msg = "Err:verifyFlashIC:Size1";
    if (read8() != 0x14)
        msg = "Err:verifyFlashIC:Size2";
	FLASH_CS_IDLE;

    // Verify the Manufacturer and device ID
    FLASH_CS_ACTIVE;
    write8(CMD_MANUFACTURER_ID); write8(0); write8(0); write8(0);
    if (read8() != 0xEF)
        msg = "Err:verifyFlashIC:ManID";
    if (read8() != 0x13)
        msg = "Err:verifyFlashIC:DevID";
	FLASH_CS_IDLE;

    if (msg) {
        SerialUSB.println(msg);
        return false;
    }
    return true;
}


// Wait until the flash IC is not busy (with timeout)
void Controleo3Flash::waitUntilNotBusy(uint16_t timeMillis)
{
    uint8_t state;
    uint32_t startTime = millis();
    while (startTime + timeMillis > millis()) {
        FLASH_CS_ACTIVE;
        write8(CMD_READ_STATUS1_REGISTER);
        state = read8();
        FLASH_CS_IDLE;
        if (!(state & STATUS_BUSY))
            return;
        delayMicroseconds(100);
    }
    SerialUSB.println("Err:waitUntilNotBusy:Timeout");
}



// Protect various parts of flash
// Either all flash, or no part of flash - or everything except the area
// where the prefs are stored.
void Controleo3Flash::protectFlash(uint8_t flashArea, bool writeToFlash)
{
    // The Status Register has a copy in RAM.  Typically, the register stored in
    // flash should always reflect that the entire flash area is protected. For
    // a lot of operations it is sufficient to temporarily remove protection for
    // the duration of that operation by just changing the register in RAM.
    // Advantages of not writing every Status Register change to flash:
    // 1. Speed - writing to flash takes 15ms
    // 2. Protection - if the device reboots part of flash may become vunerable

    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Allow changes to the Status Register
    SEND_CMD(writeToFlash? CMD_WRITE_ENABLE : CMD_VOLATILE_STATUS_REGISTER);

    FLASH_CS_ACTIVE;
    write8(CMD_WRITE_STATUS_REGISTER);

    switch (flashArea) {
        case PROTECT_ALL:
            // Protect all of the flash area.
            // CMP=0, SEC=x, TB=x, BP2=1, BP1=1 and BP0=1
            write8(STATUS_BP0 + STATUS_BP1 + STATUS_BP2);
            write8(STATUS_QE);
            break;
        case PROTECT_NONE:
            // Don't protect any of the flash area.
            // CMP=0, SEC=x, TB=x, BP2=0, BP1=0 and BP0=0
            write8(0);
            write8(STATUS_QE);
            break;
        case PROTECT_NOT_PREFS:
            // Protect everything except the area reserved for preferences and profiles (lower 128K)
            // CMP=1, SEC=0, TB=1, BP2=0, BP1=1 and BP0=0
            write8(STATUS_TB + STATUS_BP1);
            write8(STATUS_CMP + STATUS_QE);
            break;
    }
    FLASH_CS_IDLE;

    // wait for the write to complete (flash = 15ms, RAM = instantaneous)
    waitUntilNotBusy(15);
}


// Erase the entire flash IC
void Controleo3Flash::eraseFlash()
{
    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Reset the flash protection bits
    protectFlash(PROTECT_NONE, TEMPORARY_PROTECTION);

    // Enable writing to flash
    SEND_CMD(CMD_WRITE_ENABLE);

    // Erase the entire flash chip
    SEND_CMD(CMD_ERASE_FLASH);

    // Wait for the erase to complete
    waitUntilNotBusy(6000);

    // Leave the flash unprotected.  This happens anyway, but the QE bit needs to be set
    protectFlash(PROTECT_NONE, PERMANENT_PROTECTION);
}


// Erase the 4K sector holding the specified preferences
void Controleo3Flash::erasePrefsBlock(uint8_t block)
{
    // Sanity check
    if (block > 4)
      return;

    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Allow the prefs to be written to
    protectFlash(PROTECT_NOT_PREFS, TEMPORARY_PROTECTION);

    // Enable writing to flash
    SEND_CMD(CMD_WRITE_ENABLE);

    // Erase the prefs 4K sector (4K = 16 pages)
    FLASH_CS_ACTIVE;
    write8(CMD_ERASE_SECTOR_4K);
    write8(0);
    write8(block << 4);
    write8(0);
    FLASH_CS_IDLE;

    // Wait for the erase to complete
    waitUntilNotBusy(400);

    // Protect the flash again
    protectFlash(PROTECT_ALL, TEMPORARY_PROTECTION);
}


// Erase the 4K sector holding the specified profile
void Controleo3Flash::eraseProfileBlock(uint16_t block)
{
    // Sanity check
    if ((block & 0x0F) || block < 64 || block > 511) {
        SerialUSB.println("Profile block number out of range");
        return;
    }

    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Allow the prefs to be written to
    protectFlash(PROTECT_NOT_PREFS, TEMPORARY_PROTECTION);

    // Enable writing to flash
    SEND_CMD(CMD_WRITE_ENABLE);

    // Erase the prefs 4K sector (4K = 16 pages)
    FLASH_CS_ACTIVE;
    write8(CMD_ERASE_SECTOR_4K);
    write8((block & 0x0F00) >> 8);
    write8(block & 0x00FF);
    write8(0);
    FLASH_CS_IDLE;

    // Wait for the erase to complete
    waitUntilNotBusy(400);

    // Protect the flash again
    protectFlash(PROTECT_ALL, TEMPORARY_PROTECTION);
}


// Convenience function to allow writing to prefs
void Controleo3Flash::allowWritingToPrefs(boolean allow) {
    if (allow)
        protectFlash(PROTECT_NOT_PREFS, TEMPORARY_PROTECTION);
    else
        protectFlash(PROTECT_ALL, TEMPORARY_PROTECTION);
}


// Erase the lowest 128K of the flash, where the user preferences and profiles are stored
void Controleo3Flash::factoryReset()
{
    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Allow the prefs to be written to
    protectFlash(PROTECT_NOT_PREFS, TEMPORARY_PROTECTION);

    for (uint8_t i=0; i<2; i++) {
        // Enable writing to flash
        SEND_CMD(CMD_WRITE_ENABLE);

        // Erase a 64K block (64K = 256 pages)
        FLASH_CS_ACTIVE;
        write8(CMD_ERASE_BLOCK_64K);
        write8(0);
        write8(i);
        write8(0);
        FLASH_CS_IDLE;

        // Wait for the erase to complete
        waitUntilNotBusy(1000);
    }

    // Protect the flash again
    protectFlash(PROTECT_ALL, TEMPORARY_PROTECTION);
}


// Read from flash using the fastest read possible: Octal Word Read Quad I/O!
// Reads always start at the start of the page (pages are 256 bytes), so the page
// address range is 0x000 to 0xFFF.
void Controleo3Flash::startRead(uint16_t pageNumber, uint16_t bytesToRead, uint8_t *dest)
{
    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Enable Octal Word Read Quad mode
    FLASH_CS_ACTIVE;
    write8(CMD_OCTAL_WORD_READ_QUAD);

    // Put the 4 I/O pins into output mode
    setPinIOMode(PIN_IO_QUAD_WRITE);

    // Write out the address
    *portAOut = (*portAOut & 0xFFF0FFFF);   // First nibble of address is always 0 (address range is 0x00000 to 0xFFFFF)
    FLASH_PULSE_CLK;
    *portAOut = (*portAOut & 0xFFF0FFFF) + ((pageNumber & 0x000F00) << 8);
    FLASH_PULSE_CLK;
    *portAOut = (*portAOut & 0xFFF0FFFF) + ((pageNumber & 0x0000F0) << 12);
    FLASH_PULSE_CLK;
    *portAOut = (*portAOut & 0xFFF0FFFF) + ((pageNumber & 0x00000F) << 16);
    FLASH_PULSE_CLK;
    *portAOut = (*portAOut & 0xFFF0FFFF);
    FLASH_PULSE_CLK;
    FLASH_PULSE_CLK;
    FLASH_PULSE_CLK;  // M7-4
    FLASH_PULSE_CLK;  // M3-0

    // Put the 4 I/O pins into input mode
    setPinIOMode(PIN_IO_QUAD_READ);

    // Read the data
    while (bytesToRead--) {
        FLASH_PULSE_CLK;
        *dest = ((*portAIn & 0x000F0000) >> 12);
        FLASH_PULSE_CLK;
        *dest += ((*portAIn & 0x000F0000) >> 16);
        dest++;
    }
}


// Continue reading data from flash
void Controleo3Flash::continueRead(uint16_t bytesToRead, uint8_t *dest)
{
    // Read the data
    while (bytesToRead--) {
        FLASH_PULSE_CLK;
        *dest = (*portAIn & 0x000F0000) >> 12;
        FLASH_PULSE_CLK;
        *dest += (*portAIn & 0x000F0000) >> 16;
        dest++;
    }
}


// End the read from flash
void Controleo3Flash::endRead()
{
    // End the read
    FLASH_CS_IDLE;

    // Restore the I/O pins to their normal states
    setPinIOMode(PIN_IO_NORMAL);
}


// Write to flash using the fastest method possible
// Flash should be unprotected already.  See protectFlash()
// Writes always start at the start of the page (pages are 256 bytes), so the page
// address range is 0x000 to 0xFFF.
void Controleo3Flash::write(uint16_t pageNumber, uint16_t bytesToWrite, uint8_t *src)
{
    // Make sure there is something to write
    if (!bytesToWrite)
        return;

    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Enable writing to flash
    SEND_CMD(CMD_WRITE_ENABLE);

    // Enable Quad Input Page Program mode
    FLASH_CS_ACTIVE;
    write8(CMD_QUAD_INPUT_PAGE_PROGRAM);

    // Write out the address (address range is 0x00000 to 0xFFFFF)
    write8((pageNumber & 0x0F00) >> 8);
    write8(pageNumber & 0x00FF);
    write8(0);  // Always start at page boundary

    // Put the 4 I/O pins into output mode
    setPinIOMode(PIN_IO_QUAD_WRITE);

    // Write the bytes
    uint32_t zeroBits = (*portAOut & 0xFFF0FFFF);
    while (bytesToWrite--) {
        *portAOut = zeroBits + ((*src & 0xF0) << 12);
        FLASH_PULSE_CLK;
        *portAOut = zeroBits + ((*src & 0x0F) << 16);
        FLASH_PULSE_CLK;
        src++;
    }

    // End the write
    FLASH_CS_IDLE;

    // Restore the I/O pins to their normal states
    setPinIOMode(PIN_IO_NORMAL);
}


// Read the unique id (serial number) of the flash
uint32_t Controleo3Flash::readUniqueID()
{
    uint32_t id = 0;

    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Get the unique ID from the flash IC
    FLASH_CS_ACTIVE;
    write8(CMD_READ_UNIQUE_ID);
    for (int i=0; i< 4; i++)
        write8(0);
    for (int i=0; i< 4; i++) {
        id <<= 8;
        id += read8();
    }
	FLASH_CS_IDLE;
    return id;
}

struct bitmapAddressTableEntry {
    uint16_t pageToStartOfBitmap;
    uint16_t bitmapWidth;
    uint16_t bitmapHeight;
};


// Called when saving bitmaps to flash during factory initialization
// Store the bitmap information in the bitmap address table, and return the starting page
// number where the bitmap can be saved.  Bitmaps MUST be saved in sequence.
// Refer to top of this file for the flash memory map
uint16_t Controleo3Flash::getBitmapPage(uint16_t bitmapNumber, uint16_t bitmapWidth, uint16_t bitmapHeight)
{
    uint16_t tablePage, pageOffset;
    uint16_t addressTable[FLASH_C3_PAGE_SIZE >> 1]; // 128 16-bit numbers = 256 bytes

    // Bitmaps address table is saved in pages 256 to 264.  Each bitmap has a starting page, width and
    // height (6 bytes), with 42 entries per page. This allows a total of 378 bitmaps.

    // Sanity check - make sure the bitmap number is valid
    if (bitmapNumber >= FLASH_MAXIMUM_BITMAPS)
      return 0xFFFF;

    // Special case for bitmap 0
    if (bitmapNumber == 0) {
        addressTable[0] = FLASH_FIRST_BITMAP_PAGE; // Page where first bitmap is saved
        addressTable[1] = bitmapWidth;
        addressTable[2] = bitmapHeight;

        // Save the address table in flash
        write(FLASH_BITMAP_ADDRESS_TABLE, FLASH_ADDRESS_SIZE, (uint8_t *) addressTable);

        // The first bitmap gets saved to page 528
        return FLASH_FIRST_BITMAP_PAGE;
    }

    // Read the previous bitmap entry to determine where this bitmap is saved
    tablePage = FLASH_BITMAP_ADDRESS_TABLE + ((bitmapNumber - 1) / FLASH_ADDRESSES_PER_PAGE);
    pageOffset = ((bitmapNumber - 1) % FLASH_ADDRESSES_PER_PAGE) * (FLASH_ADDRESS_SIZE >> 1);
    startRead(tablePage, FLASH_C3_PAGE_SIZE, (uint8_t *) addressTable);
    endRead();
/*    SerialUSB.print("Previous bitmap: tablePage = ");
    SerialUSB.print(tablePage);
    SerialUSB.print("  pageOffset = ");
    SerialUSB.print(pageOffset);
    SerialUSB.print("  width = ");
    SerialUSB.print(addressTable[pageOffset + 1]);
    SerialUSB.print("  height = ");
    SerialUSB.println(addressTable[pageOffset + 2]);*/

    // How many pages were used to store the previous bitmap?  Multiply the width and height * 2 (16-bit)
    uint32_t bitmapBytes = addressTable[pageOffset + 1] * addressTable[pageOffset + 2] * 2;

//    if (bitmapBytes < 16)
//      SerialUSB.print("-----------------------------");
//    SerialUSB.print("Bitmap Bytes = ");
//    SerialUSB.println(bitmapBytes);
    uint16_t pagesForBitmap = (bitmapBytes + (FLASH_C3_PAGE_SIZE - 1)) >> 8;
//    SerialUSB.print("pagesForBitmap = ");
//    SerialUSB.println(pagesForBitmap);

    // Figure out the first page that this bitmap can be stored
    uint16_t pageForThisBitmap = addressTable[pageOffset] + pagesForBitmap;
//    SerialUSB.print("pageForThisBitmap = ");
//    SerialUSB.println(pageForThisBitmap);

    // Read in the address table for the current bitmap
    tablePage = FLASH_BITMAP_ADDRESS_TABLE + (bitmapNumber / FLASH_ADDRESSES_PER_PAGE);
    pageOffset = (bitmapNumber % FLASH_ADDRESSES_PER_PAGE) * (FLASH_ADDRESS_SIZE >> 1);
    startRead(tablePage, FLASH_C3_PAGE_SIZE, (uint8_t *) addressTable);
    endRead();
/*    SerialUSB.print("This bitmap: tablePage = ");
    SerialUSB.print(tablePage);
    SerialUSB.print("  pageOffset = ");
    SerialUSB.println(pageOffset);
    SerialUSB.print("  width = ");
    SerialUSB.print(bitmapWidth);
    SerialUSB.print("  height = ");
    SerialUSB.println(bitmapHeight);*/

    // Update the entry for this bitmap
    addressTable[pageOffset] = pageForThisBitmap;
    addressTable[pageOffset + 1] = bitmapWidth;
    addressTable[pageOffset + 2] = bitmapHeight;

    // Save this address table page
    write(tablePage, FLASH_C3_PAGE_SIZE, (uint8_t *) addressTable);

    return pageForThisBitmap;
}


uint16_t Controleo3Flash::getBitmapInfo(uint16_t bitmapNumber, uint16_t *bitmapWidth, uint16_t *bitmapHeight)
{
    uint16_t tablePage, pageOffset;
    uint16_t addressTable[FLASH_C3_PAGE_SIZE >> 1]; // 128 16-bit numbers = 256 bytes

    // Read in the address table for the current bitmap
    tablePage = FLASH_BITMAP_ADDRESS_TABLE + (bitmapNumber / FLASH_ADDRESSES_PER_PAGE);
    pageOffset = (bitmapNumber % FLASH_ADDRESSES_PER_PAGE) * (FLASH_ADDRESS_SIZE >> 1);
    startRead(tablePage, FLASH_C3_PAGE_SIZE, (uint8_t *) addressTable);
    endRead();

    *bitmapWidth = addressTable[pageOffset + 1];
    *bitmapHeight = addressTable[pageOffset + 2];
    return addressTable[pageOffset];
}


// Write 8 bits to flash
void Controleo3Flash::write8(uint8_t data)
{
	for (uint8_t count=0; count<8; count++)
	{
		if (data & 0x80)
			FLASH_MOSI_ACTIVE;
		else
			FLASH_MOSI_IDLE;
		data = data << 1;
		FLASH_PULSE_CLK;
	}
}


// Read 8 bits from flash
uint8_t Controleo3Flash::read8()
{
	uint8_t data = 0;

	for (uint8_t count=0; count<8; count++)
	{
		data <<= 1;
		FLASH_PULSE_CLK;
		if (FLASH_MISO_HIGH)
			data++;
	}
	return data;
}


// One-bit read
void Controleo3Flash::slowRead(uint16_t pageNumber, uint16_t bytesToRead, uint8_t *dest)
{
    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Enable Octal Word Read Quad mode
    FLASH_CS_ACTIVE;
    write8(3);

    // Write out the address (address range is 0x00000 to 0xFFFFF)
    write8((pageNumber & 0x0F00) >> 8);
    write8(pageNumber & 0x00FF);
    write8(0);  // Always start at page boundary

    // Read the data
    while (bytesToRead--) {
        *dest = read8();
        dest++;
    }
    // End the read
    FLASH_CS_IDLE;
}


// One bit write
void Controleo3Flash::slowWrite(uint16_t pageNumber, uint16_t bytesToWrite, uint8_t *src)
{
    // Make sure previous commands have finished executing
    waitUntilNotBusy(50);

    // Enable writing to flash
    SEND_CMD(CMD_WRITE_ENABLE);

    // Enable Quad Input Page Program mode
    FLASH_CS_ACTIVE;
    write8(2);

    // Write out the address (address range is 0x00000 to 0xFFFFF)
    write8((pageNumber & 0x0F00) >> 8);
    write8(pageNumber & 0x00FF);
    write8(0);  // Always start at page boundary

    // Write the bytes
    while (bytesToWrite--) {
        write8(*src);
        src++;
    }

    // End the write
    FLASH_CS_IDLE;
}


// Dump the flash status registers.  Used for debugging
void Controleo3Flash::dumpStatusRegisters()
{
    FLASH_CS_ACTIVE;
    write8(CMD_READ_STATUS1_REGISTER);
    uint8_t s2 = read8();
    FLASH_CS_IDLE;
    SerialUSB.print("Status Register 1 = 0x"); SerialUSB.print(s2, HEX);
    FLASH_CS_ACTIVE;
    write8(CMD_READ_STATUS2_REGISTER);
     s2 = read8();
    FLASH_CS_IDLE;
    SerialUSB.print("  Status Register 2 = 0x"); SerialUSB.println(s2, HEX);
}
