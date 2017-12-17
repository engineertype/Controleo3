// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//

#define MAX_CALIBRATION_TIME  60000  // 60 seconds
#define MAX_TAP_TARGETS    20
//#define SHOW_TAP_TARGETS

static struct  {
  uint16_t left;
  uint16_t top;
  uint16_t right;
  uint16_t bottom;
} tapTarget[MAX_TAP_TARGETS];

static uint8_t touchNumTargets = 0;
static void (*touchCallbackFunction) ();
static uint16_t touchCallbackInterval;
static void (*touchCallbackTemperatureUnitChange) (boolean);
static uint8_t touchScreenshotTaps = 0;
static boolean touchDisplayInCelsius = true;

boolean drawTemperatureOnScreenNow;

// Calibrate the touch screen
void CalibrateTouchscreen()
{
  uint32_t calibrationStartTime = millis();
  int16_t i, topLeftX, topLeftY, topRightX, topRightY, bottomLeftX, bottomLeftY, bottomRightX, bottomRightY, centerX, centerY;

  // Clear the screen
  tft.fillScreen(WHITE);

restart:
  while (1) {
    displayString(141, 10, FONT_9PT_BLACK_ON_WHITE, (char *) "Tap the target to");
    displayString(125, 40, FONT_9PT_BLACK_ON_WHITE, (char *) "calibrate the screen");
    displayString(64, 290, FONT_9PT_BLACK_ON_WHITE, (char *) "Use a stylus for best accuracy");
    
    // Draw crosshairs on the top-left corner of the screen
    drawCrosshairs(40, 40, true);
    // Debounce touch
    calibrationDebounce();
    while (!touch.readRaw(&topLeftX, &topLeftY) && millis() - calibrationStartTime <= MAX_CALIBRATION_TIME)
      delay(5);
    // Abort if calibration has taken too long
    if (millis() - calibrationStartTime > MAX_CALIBRATION_TIME)
      return;
    // Erase the crosshairs
    drawCrosshairs(40, 40, false);
    playTones(TUNE_BUTTON_PRESSED);
    // Sanity check on test point
    if (topLeftX < 2800 || topLeftY < 2700)
      continue;

    // Draw crosshairs on the top-right corner of the screen
    drawCrosshairs(440, 40, true);
    // Debounce touch
    calibrationDebounce();
    while (!touch.readRaw(&topRightX, &topRightY) && millis() - calibrationStartTime <= MAX_CALIBRATION_TIME)
      delay(5);
    // Abort if calibration has taken too long
    if (millis() - calibrationStartTime > MAX_CALIBRATION_TIME)
      return;
    // Erase the crosshairs
    drawCrosshairs(440, 40, false);
    playTones(TUNE_BUTTON_PRESSED);
    // Sanity check on test point
    if (topRightX > 1000 || topRightY < 2700)
      continue;
      
    // Draw crosshairs on the bottom-right corner of the screen
    drawCrosshairs(440, 280, true);
    // Debounce touch
    calibrationDebounce();
    while (!touch.readRaw(&bottomRightX, &bottomRightY) && millis() - calibrationStartTime <= MAX_CALIBRATION_TIME)
      delay(5);
    // Abort if calibration has taken too long
    if (millis() - calibrationStartTime > MAX_CALIBRATION_TIME)
      return;
    // Erase the crosshairs
    drawCrosshairs(440, 280, false);
    playTones(TUNE_BUTTON_PRESSED);
    // Sanity check on test point
    if (bottomRightX > 1000 || bottomRightY > 1100)
      continue;

    // Draw crosshairs on the bottom-left corner of the screen
    drawCrosshairs(40, 280, true);
    // Debounce touch
    calibrationDebounce();
    while (!touch.readRaw(&bottomLeftX, &bottomLeftY) && millis() - calibrationStartTime <= MAX_CALIBRATION_TIME)
      delay(5);
    // Abort if calibration has taken too long
    if (millis() - calibrationStartTime > MAX_CALIBRATION_TIME)
      return;
    // Erase the crosshairs
    drawCrosshairs(40, 280, false);
    playTones(TUNE_BUTTON_PRESSED);
    // Sanity check on test point
    if (bottomLeftX < 2800 || bottomLeftY > 1100)
      continue;

    // Draw crosshairs in the center of the screen
    drawCrosshairs(240, 160, true);
    // Debounce touch
    calibrationDebounce();
    while (!touch.readRaw(&centerX, &centerY) && millis() - calibrationStartTime <= MAX_CALIBRATION_TIME)
      delay(5);
    // Abort if calibration has taken too long (1 minute)
    if (millis() - calibrationStartTime > MAX_CALIBRATION_TIME)
      return;
    // Erase the crosshairs
    drawCrosshairs(240, 160, false);
    playTones(TUNE_BUTTON_PRESSED);
    // Sanity check on test point
    int16_t averageX = (topLeftX + topRightX + bottomRightX + bottomLeftX) >> 2;
    int16_t averageY = (topLeftY + topRightY + bottomRightY + bottomLeftY) >> 2;
    SerialUSB.print(F("X delta = "));
    SerialUSB.println(abs(averageX - centerX));
    SerialUSB.print(F("Y delta = "));
    SerialUSB.println(abs(averageY - centerY));
    if (abs(averageX - centerX) > 60 || abs(averageY - centerY) > 60) {
      displayString(183, 240, FONT_9PT_BLACK_ON_WHITE, (char *) "Try again!");
      continue;
    }

    // Done with collecting touch points now
    // Calculate the extremes of the touch panel
    // The X-axis is 480 pixels with targets at 40 and 440 
    int16_t fourtyPixels = (topLeftX - topRightX) / 10;
    prefs.topLeftX = topLeftX + fourtyPixels;
    prefs.topRightX = topRightX - fourtyPixels;

    fourtyPixels = (bottomLeftX - bottomRightX) / 10;
    prefs.bottomLeftX = bottomLeftX + fourtyPixels;
    prefs.bottomRightX = bottomRightX - fourtyPixels;

    // The Y-axis is 320 pixels, with targets at 40 and 280
    fourtyPixels = (topLeftY - bottomLeftY) / 6;
    prefs.topLeftY = topLeftY + fourtyPixels;
    prefs.bottomLeftY = bottomLeftY - fourtyPixels;
  
    fourtyPixels = (topRightY - bottomRightY) / 6;
    prefs.topRightY = topRightY + fourtyPixels;
    prefs.bottomRightY = bottomRightY - fourtyPixels;

    // Send the calibration data to the touch driver
    sendTouchCalibrationData();

    // Erase the text on the screen
    tft.fillRect(125, 10, 230, 50, WHITE);
    tft.fillRect(183, 240, 114, 25, WHITE);
    tft.fillRect(64, 290, 360, 25, WHITE);
    
    // Draw some text and buttons on the screen
    displayString(21, 10, FONT_9PT_BLACK_ON_WHITE, (char *) "Draw on the screen to test calibration");
    clearTouchTargets();
    drawTouchButton(130, 100, 220, 181, BUTTON_LARGE_FONT, (char *) "Recalibrate");
    drawTouchButton(130, 180, 220, 82, BUTTON_LARGE_FONT, (char *) "Done");
    
    // Save the touch calibration settings immediately
    writePrefsToFlash();
    
    // Debounce touch
    calibrationDebounce();

    // Loop here as the users tests drawing
    while (1) {
      delay(2);
      if (!touch.read(&centerX, &centerY))
        continue;
      // Draw a blob where the touch was
      tft.fillRect(centerX-1, centerY-1, 3, 3, RED);

      // See if the tap is in a valid area
      for (i=0; i< touchNumTargets; i++) {
        if (centerX < tapTarget[i].left || centerX > tapTarget[i].right)
          continue;
        if (centerY < tapTarget[i].top || centerY > tapTarget[i].bottom)
          continue;

        // Tap was in target area!!
        playTones(TUNE_BUTTON_PRESSED);

        if (i == 0) {
          // Recalibrate button
          // Clear the screen
          tft.fillScreen(WHITE);
          // Restart the abort calibration timer
          calibrationStartTime = millis();
          goto restart;
        }
        else {
          // Done button
          return;
        }
      }
    }
  }  
}


// Draw the calibration crosshairs on the screen
void drawCrosshairs(uint16_t x, uint16_t y, boolean draw)
{
  tft.fillRect(x-1, y-20, 3, 41, draw? BLUE: WHITE);
  tft.fillRect(x-20, y-1, 41, 3, draw? BLUE: WHITE);
}


// Debounce touches, but allow for tap-and-hold
void debounce()
{
  uint32_t now, lastTouch, startTime = millis();
  lastTouch = startTime;

  while (1) {
    now = millis();
    // Don't debounce for too long (need to support tap & hold)
    if (now - startTime > 150)
      return;
    // Is there a touch now?
    if (touch.isPressed())
      lastTouch = now;
    else {
      // Wait for 50ms after last touch to ensure debounce
      if (now - lastTouch > 50)
        return;
    }
    delay(1);
  }
}


// Debounce, but with smaller times (used for non-critical changes, like changing C to F)
void quickDebounce()
{
  uint32_t now, lastTouch, startTime = millis();
  lastTouch = startTime;
  while (1) {
    now = millis();
    // Don't debounce for too long
    if (now - startTime > 100)
      return;
    // Is there a touch now?
    if (touch.isPressed())
      lastTouch = now;
    else {
      // Wait for 30ms after last touch to ensure debounce
      if (now - lastTouch > 30)
        return;
    }
    delay(1);
  }
}


// Wait for 250ms after the last touch
void calibrationDebounce()
{
  uint32_t lastTouch = millis();
  delay(100);
  while(1) {
    // Time to exit?
    if (millis() - lastTouch > 250)
      return;
    // Was there a touch?
    if (touch.isPressed())
      lastTouch = millis();
    delay(5);
  }
}


// Send the calibration data to the touch driver
void sendTouchCalibrationData()
{
    touch.calibrate(prefs.topLeftX,prefs.topRightX,prefs.bottomLeftX,prefs.bottomRightX,prefs.topLeftY,prefs.bottomLeftY,prefs.topRightY,prefs.bottomRightY);
}


// The next section deals with handling user taps on the screen

// Clear all tap data
void clearTouchTargets()
{
  touchNumTargets = 0;
  touchCallbackFunction = 0;
  touchCallbackInterval = 0;
  touchCallbackTemperatureUnitChange = 0;
  touchScreenshotTaps = 0;
  touchDisplayInCelsius = true;
}


// Callback function to call while waiting for touch.  Inverval is in milliseconds
void setTouchIntervalCallback(void (*f) (), uint16_t interval)
{
  touchCallbackFunction = f;
  touchCallbackInterval = interval;
}


// Callback function when user taps in top-right corner to change temperature units
void setTouchTemperatureUnitChangeCallback(void (*f) (boolean displayInCelsius))
{
  touchCallbackTemperatureUnitChange = f;
}


// Define touch areas on the screen.  None must be in the top 45 pixels of the screen
// Two areas are always defined:
// 1. Tap in top left corner to take screenshot
// 2. Tap in top right corner to switch temperature reading between C and F
void defineTouchArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  if (touchNumTargets >= MAX_TAP_TARGETS) {
    SerialUSB.println("Too many tap targets!!");
    return;
  }
  
  tapTarget[touchNumTargets].left = x;
  tapTarget[touchNumTargets].top = y;
  tapTarget[touchNumTargets].right = x + w;
  tapTarget[touchNumTargets].bottom = y + h;

#ifdef SHOW_TAP_TARGETS
  tft.drawRect(x, y, w, h, RED);
#endif

  touchNumTargets++;
}


int8_t getTap(uint8_t mode)
{
  int16_t x, y;
  static uint32_t timeOfLastTemperatureUpdate = 0;
  boolean showTemperatureInHeader = (mode == SHOW_TEMPERATURE_IN_HEADER);

  // Show the temperature in the header as soon as possible
  if (showTemperatureInHeader && drawTemperatureOnScreenNow) {
    drawTemperatureOnScreenNow = false;
    displayTemperatureInHeader();
    if (touchCallbackTemperatureUnitChange)
      (*touchCallbackTemperatureUnitChange) (touchDisplayInCelsius);
  }
  // Debounce any taps that took us to this screen
  if (mode != CHECK_FOR_TAP_THEN_EXIT)
    debounce();

  // Repeat until valid tap (or just once if mode is CHECK_FOR_TAP_THEN_EXIT)
  while (1) {
    // See if prefs should be written to flash.  The write is time-delayed to reduce flash write cycles
    checkIfPrefsShouldBeWrittenToFlash();

    // Poll for valid tap reading
    if (!touch.read(&x, &y))  {
      // Exit if this is all the calling function wanted
      if (mode == CHECK_FOR_TAP_THEN_EXIT)
        return -1;
      
      // Update the temperature display every second
      if (showTemperatureInHeader) {
        if (millis() - timeOfLastTemperatureUpdate > 1000) {
          timeOfLastTemperatureUpdate = millis();
          displayTemperatureInHeader();
        }
      }

      // Call the callback routine
      touchCallback();
      delay(1);
      continue;
    }

    // See if the tap is in a valid area
    for (uint8_t i=0; i< touchNumTargets; i++) {
      if (x < tapTarget[i].left || x > tapTarget[i].right)
        continue;
      if (y < tapTarget[i].top || y > tapTarget[i].bottom)
        continue;

      // Tap was in target area!!
      playTones(TUNE_BUTTON_PRESSED);
      return i;
    }

    // See if this is a screenshot tap
    if (x < 60 && y < 45) {
      touchScreenshotTaps++;
      if (touchScreenshotTaps % 3 == 0) {
        // Take a screenshot now
        takeScreenshot();
      }
      debounce();
    }

    // If this if in the top right corner, toggle the temperature between Celsius and Farenheit
    if (mode != DONT_SHOW_TEMPERATURE && x > 340 && y < 50) {
      touchDisplayInCelsius = !touchDisplayInCelsius;
      displayTemperatureInHeader();
      if (touchCallbackTemperatureUnitChange)
        (*touchCallbackTemperatureUnitChange) (touchDisplayInCelsius);
      playTones(TUNE_BUTTON_PRESSED);
      quickDebounce();
    }
  }

  // Should never get here
  return 0;
}


void touchCallback() 
{
  static uint32_t lastCalled = 0;
  uint32_t now;

  // Is there a callback routine?
  if (!touchCallbackFunction || !touchCallbackInterval)
    return;

  // Is it time to call the callback?
  now = millis();
  if (now - lastCalled > touchCallbackInterval) {
    // Time to call the callback routine
    (*touchCallbackFunction)();
    lastCalled = now;
  }
}


// Display the temperature on the screen once per second
void displayTemperatureInHeader()
{
  float temperature = getCurrentTemperature();
  char *str = getTemperatureString(buffer100Bytes, temperature, touchDisplayInCelsius);

  // Display the temperature
  if (IS_MAX31856_ERROR(temperature)) {
    tft.fillRect(366, 11, 110, 19, WHITE);
    displayString(418, 11, FONT_9PT_BLACK_ON_WHITE_FIXED, str);
  }
  else {
    displayFixedWidthString(366, 11, str, 8, FONT_9PT_BLACK_ON_WHITE_FIXED);
  }
}

