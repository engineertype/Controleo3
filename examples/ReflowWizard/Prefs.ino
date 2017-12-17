// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//

// Preferences are stored in 4K blocks.  There are 4 blocks, and they are rotated for each write to
// maximixe redundancy and minimize write wear.  Blocks have a 32-bit sequence number as the first
// 4 bytes.  This number is increased after each write to be able to identify the latest prefs.
// Blocks are initialized to 0xFF's after erase, so prefrences should be added with this in mind.


#define NO_OF_PREFS_BLOCKS          4
#define PAGES_PER_PREFS_BLOCK       16

uint8_t lastPrefsBlock = 0;
uint32_t timeOfLastSavePrefsRequest = 0;

void getPrefs() 
{
  // Sanity check on the size of the prefences
  if (sizeof(prefs) > 4096)
  {
    SerialUSB.println("Prefs size must not exceed 4Kb!!!!!!!!");
    // Hopefully getting a massive delay while developing new code alert you to this situation 
    delay(10000);
  }
  
  // Get the prefs with the highest sequence number
  uint32_t highestSequenceNumber = 0, seqNo;
  uint8_t prefsToUse = 0;
  for (uint8_t i=0; i < NO_OF_PREFS_BLOCKS; i++) {
      flash.startRead(i * PAGES_PER_PREFS_BLOCK, sizeof(uint32_t), (uint8_t *) &seqNo);
      flash.endRead();
      // Skip blocks that are erased
      if (seqNo == 0xFFFFFFFF)
        continue;
      // Is this the highest sequence number?
      if (seqNo <= highestSequenceNumber)
        continue;
      // So far, this is the prefs block to use
      highestSequenceNumber = seqNo;
      prefsToUse = i;
  }

  // Read all the prefs in now
  flash.startRead(prefsToUse * PAGES_PER_PREFS_BLOCK, sizeof(Controleo3Prefs), (uint8_t *) &prefs);
  flash.endRead();

  // If this is the first time the prefs are read in, initialize them
  if (prefs.sequenceNumber == 0xFFFFFFFF) {
    // Initialize the whole of prefs to zero.
    uint8_t *p = (uint8_t *) &prefs;
    for (uint32_t i=0; i < sizeof(prefs); i++)
      *p++ = 0;

    // Only need to initialize non-zero prefs here
    // Door servo positions
    prefs.servoOpenDegrees = 90;
    prefs.servoClosedDegrees = 90;
    prefs.bakeTemperature = BAKE_MIN_TEMPERATURE;
    prefs.bakeDuration = 31;  // 1 hour
    prefs.openDoorAfterBake = BAKE_DOOR_OPEN_CLOSE_COOL;

    prefs.lastUsedProfileBlock = FIRST_PROFILE_BLOCK;
  }

  SerialUSB.println("Read prefs from block " + String(prefsToUse) + ". Seq No = " + String(prefs.sequenceNumber));

/* Defaults for oven in build guide
  prefs.learningComplete = true;
  prefs.learnedPower[0] = 13;
  prefs.learnedPower[1] = 32;
  prefs.learnedPower[2] = 28;
  prefs.learnedInertia[0] = 41;
  prefs.learnedInertia[1] = 136;
  prefs.learnedInertia[2] = 57;
  prefs.learnedInsulation = 124;*/
  
  // Remember which block was last used to save prefs
  lastPrefsBlock = prefsToUse;
}


// Save the prefs to external flash
void savePrefs()
{
  // Don't write prefs immediately.  Wait a short time in case the user is moving quickly between items
  // Doing this will reduce the number of writes to the flash, saving flash wear.
  // It also makes "tap and hold" more responsive.
  timeOfLastSavePrefsRequest = millis();
}


// This is called while waiting for the user to tap the screen
void checkIfPrefsShouldBeWrittenToFlash()
{
  // Is there a pending prefs write?
  if (timeOfLastSavePrefsRequest == 0)
    return;
  // Write to flash fairly soon after the last change
  if (millis() - timeOfLastSavePrefsRequest > 3000) {
    writePrefsToFlash();
    timeOfLastSavePrefsRequest = 0;
  }
}


// Flash writes are good for 50,000 cycles - and there are 4 blocks used for prefs = 200,000 cycles.  
void writePrefsToFlash() 
{
  // Increase the preference sequence number
  prefs.sequenceNumber++;

  // Prefs get stored in the next block (not the current one).  This reduces flash wear, and adds some redundancy
  lastPrefsBlock = (lastPrefsBlock + 1) % NO_OF_PREFS_BLOCKS;

  // Erase the block the prefs will be stored to
  flash.erasePrefsBlock(lastPrefsBlock);

  // Save the preferences, one page (256 bytes) at a time
  uint16_t prefsSize = sizeof(Controleo3Prefs);

  // Sanity check on prefs size (maximum is 4K)
  if (prefsSize > 4096) {
    SerialUSB.println("Prefs exceed the 4K maximum!!!");
    return;
  }

  // Allow the prefs to be written to
  flash.allowWritingToPrefs(true);

  // There are 16 x 256 byte pages in 4K
  uint16_t page = lastPrefsBlock * PAGES_PER_PREFS_BLOCK;
  uint8_t *p = (uint8_t *) &prefs;
  while (prefsSize > 0) {
    flash.write(page++, prefsSize > 256? 256: prefsSize, p);
    prefsSize = prefsSize > 256? prefsSize - 256: 0;
    p += 256;
  }
  // Protect flash again, now that writing is done
  flash.allowWritingToPrefs(false);

  SerialUSB.println("Finished writing prefs to block " + String(lastPrefsBlock) + ". Seq No = " + String(prefs.sequenceNumber));
  timeOfLastSavePrefsRequest = 0;
}


// This performs a factory reset, erasing preferences and profiles
void factoryReset(boolean saveTouchCalibrationData)
{
  uint32_t sequenceNumber = prefs.sequenceNumber;
  
  // Save the touchscreen calibration data
  memcpy(buffer100Bytes, &prefs.topLeftX, 16);
  flash.factoryReset(); 
  // Get the factory-default prefs from flash
  getPrefs();
  // Restore the touchscreen data if touchscreen calibration data should be saved
  if (saveTouchCalibrationData)
    memcpy(&prefs.topLeftX, buffer100Bytes, 16);
  // Restore sequence number
  prefs.sequenceNumber = sequenceNumber;
  writePrefsToFlash();
}

