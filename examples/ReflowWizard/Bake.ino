// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//

// Stay in this function until the bake is done or canceled
void bake() {
  uint32_t secondsLeftOfBake, lastLoopTime = millis();
  uint8_t counter = 0;
  uint8_t bakePhase = BAKING_PHASE_HEATUP;
  double currentTemperature = getCurrentTemperature();
  uint8_t elementDutyCounter[NUMBER_OF_OUTPUTS];
  boolean isOneSecondInterval = false;
  uint16_t iconsX, i;
  uint8_t bakeDutyCycle, bakeIntegral = 0, coolingDuration = 0;
  boolean isHeating = true;
  long lastOverTempTime = 0;
  boolean abortDialogIsOnScreen = false;
  
  // Verify the outputs are configured
  if (areOutputsConfigured() == false) {
    showHelp(HELP_OUTPUTS_NOT_CONFIGURED);
    return;
  }

  // Initialize varaibles used for baking
  secondsLeftOfBake = getBakeSeconds(prefs.bakeDuration);
  // Start with a duty cycle proportional to the desired temperature
  bakeDutyCycle = map(prefs.bakeTemperature, 0, 250, 0, 100);

  // Calculate the centered position of the heating and fan icons (icons are 32x32)
  iconsX = 240 - (numOutputsConfigured() * 20) + 4;  // (2*20) - 32 = 8.  8/2 = 4

  // Turn on any convection fans
  setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_ON, COOLING_FAN_OFF);

  // Stagger the element start cycle to avoid abrupt changes in current draw
  // Simple method: there are 6 outputs but the first ones are likely the heating elements
  for (i=0; i< NUMBER_OF_OUTPUTS; i++)
    elementDutyCounter[i] = (70 * i) % 100;

  // Set up the screen in preparation for baking
  // Erase the bottom part of the screen
  tft.fillRect(0, 100, 480, 220, WHITE);

  // Ug, hate goto's!  But this saves a lot of extraneous code.
userChangedMindAboutAborting:

  // Setup the tap targets on this screen
  clearTouchTargets();
  drawButton(110, 230, 260, 87, BUTTON_LARGE_FONT, (char *) "STOP");
  defineTouchArea(20, 150, 440, 170); // Large tap target to stop baking

  // Toggle the baking temperature between C/F if the user taps in the top-right corner
  setTouchTemperatureUnitChangeCallback(displayBakeTemperatureAndDuration);

  // Display baking information to the screen and for debugging
  displayBakeTemperatureAndDuration(true);
  SerialUSB.println(buffer100Bytes);

  // Display the bake phase on the screen
  displayBakePhase(bakePhase, abortDialogIsOnScreen);

  // Debounce any taps that took us to this screen
  debounce();

  // Keep looping until baking is done
  while (1) {
    // Has there been a touch?
    switch (getTap(CHECK_FOR_TAP_THEN_EXIT)) {
      case 0: 
        // If baking is done (or user taps "stop" in Abort dialog) then clean up and return to the main menu
        if (bakePhase >= BAKING_PHASE_COOLING || abortDialogIsOnScreen) {
          bakePhase = BAKING_PHASE_ABORT;
          // Make sure we exit this screen as soon as possible
          lastLoopTime = millis() - 20;
          counter = 40;
        }
        else {
          // User tapped to abort baking
          drawBakingAbortDialog();
          abortDialogIsOnScreen = true;
        }
        break;

      default:
        // The user didn't tap the screen, but if the Abort dialog is up and the phase makes
        // it irrelevant then automatically dismiss it now
        // You never know, maybe the cat tapped "Stop" ...
        if (!abortDialogIsOnScreen || bakePhase < BAKING_PHASE_COOLING)
          break;
        // Intentional fall-through (simulate user tapped Cancel) ...

      case 1:
        // This is the cancel button of the Abort dialog.  User wants to continue
        // Erase the Abort dialog
        tft.fillRect(0, 90, 480, 230, WHITE);
        abortDialogIsOnScreen = false;
        counter = 0;
        // Redraw the screen under the dialog
        goto userChangedMindAboutAborting;
    }
    
    // Execute this loop every 20ms (50 times per second)
    if (millis() - lastLoopTime < 20) {
      delay(1);
      continue;
    }
    lastLoopTime += 20;

    // Try not to update everything in the same 20ms time slice
    // Update the countdown clock
    if (counter == 0 && !abortDialogIsOnScreen)
      displayBakeSecondsLeft(secondsLeftOfBake);
    // Update the temperature
    if (counter == 2)
      displayTemperatureInHeader();
    // Dump data to the debugging port
    if (counter == 5 && bakePhase != BAKING_PHASE_DONE)
      DisplayBakeTime(secondsLeftOfBake, currentTemperature, bakeDutyCycle, bakeIntegral);

    // Determine if this is on a 1-second interval
    isOneSecondInterval = false;
    if (++counter >= 50) {
      counter = 0;
      isOneSecondInterval = true;
    }
    
    // Read the current temperature
    currentTemperature = getCurrentTemperature();
    if (IS_MAX31856_ERROR(currentTemperature)) {
      switch ((int) currentTemperature) {
        case FAULT_OPEN:
          strcpy(buffer100Bytes, "Fault open (disconnected)");
          break;
        case FAULT_VOLTAGE:
          strcpy(buffer100Bytes, "Over/under voltage (wrong type?)");
          break;
        case NO_MAX31856: // Should never happen unless MAX31856 is broken
          strcpy(buffer100Bytes, "MAX31856 error");
          break;
      }
    
      // Abort the bake
      SerialUSB.println("Thermocouple error:" + String(buffer100Bytes));
      SerialUSB.println("Bake aborted because of thermocouple error!");
      // Show the error on the screen
      drawThickRectangle(0, 90, 480, 230, 15, RED);
      tft.fillRect(15, 105, 450, 100, WHITE);
      displayString(130, 110, FONT_12PT_BLACK_ON_WHITE, (char *) "Baking Error!");
      displayString(40, 150, FONT_9PT_BLACK_ON_WHITE, (char *) "Thermocouple error:");
      displayString(40, 180, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
      // Turn everything off
      setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_OFF, COOLING_FAN_OFF);
      animateIcons(iconsX); 
      // Wait for the user to tap the screen
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      bakePhase = BAKING_PHASE_ABORT;
    }

    switch (bakePhase) {
      case BAKING_PHASE_HEATUP:
        // Don't start decrementing secondsLeftOfBake until close to baking temperature
        // Is the oven close to the desired temperature?
        if (prefs.bakeTemperature - currentTemperature < 15.0) {
          bakePhase = BAKING_PHASE_BAKE;
          displayBakePhase(bakePhase, abortDialogIsOnScreen);
          // Reduce the duty cycle for the last 15 degrees
          bakeDutyCycle = bakeDutyCycle / 3;
          SerialUSB.println("Move to bake phase");
        }
        break;
       
      case BAKING_PHASE_BAKE:
        // Make changes every second
        if (!isOneSecondInterval)
          break;
          
        // Has the bake duration been reached?
        secondsLeftOfBake--;
        if (secondsLeftOfBake == 0) {
          // Successful bake.  Make a note of this
          prefs.numBakes++;
          savePrefs();
          bakePhase = BAKING_PHASE_START_COOLING;
          break;
        }

        // Is the oven too hot?
        if (currentTemperature > prefs.bakeTemperature) {
          if (isHeating) {
            isHeating = false;
            // Turn all heating elements off
            setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_ON, COOLING_FAN_OFF);

            // The duty cycle caused the temperature to exceed the bake temperature, so decrease it
            // (but not more than once every 30 seconds)
            if (millis() - lastOverTempTime > (30 * 1000)) {
              lastOverTempTime = millis();
              if (bakeDutyCycle > 0)
                bakeDutyCycle--;
            }

            // Reset the bake integral, so it will be slow to increase the duty cycle again
            bakeIntegral = 0;
            SerialUSB.println("Over-temp. Elements off");
          }
          // No more to do here
          break;
        }

        // The oven is heating up
        isHeating = true;

        // Increase the bake integral if not close to temperature
        if (prefs.bakeTemperature - currentTemperature > 1.0)
          bakeIntegral++;
          
        // Has the oven been under-temperature for a while?
        if (bakeIntegral > 30) {
          bakeIntegral = 0;
          // Increase duty cycles
          if (bakeDutyCycle < 100)
            bakeDutyCycle++;
          SerialUSB.println("Under-temp. Increasing duty cycle");
        }
        break;

      case BAKING_PHASE_START_COOLING:
        isHeating = false;
      
        // Turn off all elements and turn on the fans
        setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_ON, COOLING_FAN_ON);
     
        // Move to the next phase
        bakePhase = BAKING_PHASE_COOLING;
        displayBakePhase(bakePhase, abortDialogIsOnScreen);

        // If a servo is attached, use it to open the door over 10 seconds
        if (prefs.openDoorAfterBake != BAKE_DOOR_LEAVE_CLOSED)
          setServoPosition(prefs.servoOpenDegrees, 10000);

        // Change the STOP button to DONE
        tft.fillRect(150, 242, 180, 36, WHITE);
        drawButton(110, 230, 260, 93, BUTTON_LARGE_FONT, (char *) "DONE");
        
        // Cooling should be at least 60 seconds in duration
        coolingDuration = 60;
        break;

      case BAKING_PHASE_COOLING:
        // Make changes every second
        if (!isOneSecondInterval)
          break;
          
        // Wait in this phase until the oven has cooled
        if (coolingDuration > 0)
          coolingDuration--;      
        if (currentTemperature < 50.0 && coolingDuration == 0) {
          isHeating = false;
          // Turn all elements and fans off
          setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_OFF, COOLING_FAN_OFF);
          // Close the oven door now, over 3 seconds
          if (prefs.openDoorAfterBake == BAKE_DOOR_OPEN_CLOSE_COOL)
            setServoPosition(prefs.servoClosedDegrees, 3000);
          // Stay on this screen and wait for the user tap
          bakePhase = BAKING_PHASE_DONE;
          displayBakePhase(bakePhase, abortDialogIsOnScreen);
          // Play a tune to let the user know baking is done
          playTones(TUNE_REFLOW_DONE);
        }
        break;

      case BAKING_PHASE_DONE:
        // Nothing to do here.  Just waiting for user to tap the screen
        break;

      case BAKING_PHASE_ABORT:
        SerialUSB.println("Bake is over!");
        // Turn all elements and fans off
        setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_OFF, COOLING_FAN_OFF);
        // Close the oven door now, over 3 seconds
        setServoPosition(prefs.servoClosedDegrees, 3000);
        // Return to the main menu
        return;
    }
 
    // Turn the outputs on or off based on the duty cycle
    if (isHeating) {
      for (i=0; i< NUMBER_OF_OUTPUTS; i++) {
        switch (prefs.outputType[i]) {
          case TYPE_TOP_ELEMENT:
            // Turn the output on at 0, and off at the duty cycle value
            if (elementDutyCounter[i] == 0)
              setOutput(i, HIGH);
            // Restrict the top element's duty cycle to 75% to protect the insulation
            // and reduce IR heating of components
            if (elementDutyCounter[i] == (bakeDutyCycle < 75? bakeDutyCycle: 75))
              setOutput(i, LOW);
            break;
          case TYPE_BOTTOM_ELEMENT:
            // Turn the output on at 0, and off at the duty cycle value
            if (elementDutyCounter[i] == 0)
              setOutput(i, HIGH);
            if (elementDutyCounter[i] == bakeDutyCycle)
              setOutput(i, LOW);
            break;
          
          case TYPE_BOOST_ELEMENT: 
            // Give it half the duty cycle of the other elements
            // Turn the output on at 0, and off at the duty cycle value
            if (elementDutyCounter[i] == 0)
              setOutput(i, HIGH);
            if (elementDutyCounter[i] == bakeDutyCycle/2)
              setOutput(i, LOW);
          break;
        }
      
        // Increment the duty counter
        elementDutyCounter[i] = (elementDutyCounter[i] + 1) % 100;
      }
    }

    animateIcons(iconsX);  
  } // end of big while loop
}


// Print baking information to the serial port so it can be plotted
void DisplayBakeTime(uint16_t duration, float temperature, int duty, int integral) {
  // Write the time and temperature to the serial port, for graphing or analysis on a PC
  uint16_t fraction = ((uint16_t) (temperature * 100)) % 100;
  sprintf(buffer100Bytes, "%u, %d.%02d, %i, %i", duration, (uint16_t) temperature, fraction, duty, integral);
  SerialUSB.println(buffer100Bytes);
}


// Display the baking phase on the screen
void displayBakePhase(uint8_t phase, boolean abortDialogIsOnScreen)
{
  static uint16_t lastMsgX = 0, lastLen = 1;
  // Don't display anything if the abort dialog is on the screen
  if (abortDialogIsOnScreen)
    return;
  // Ease the previous message
  tft.fillRect(lastMsgX, 175, lastLen, 24, WHITE);
  // Display the new message
  lastLen = displayString(bakePhaseStrPosition[phase], 175, FONT_9PT_BLACK_ON_WHITE, (char *) bakePhaseStr[phase]);
  lastMsgX = bakePhaseStrPosition[phase];
  // Dump this out the debugging port too
  SerialUSB.println("Baking phase = " + String(bakePhaseStr[phase]));
}


// Draw the abort dialog on the screen.  The user needs to confirm that they want to exit bake
void drawBakingAbortDialog()
{
  drawThickRectangle(0, 90, 480, 230, 15, RED);
  tft.fillRect(15, 105, 450, 200, WHITE);
  displayString(140, 110, FONT_12PT_BLACK_ON_WHITE, (char *) "Stop Baking");
  displayString(62, 150, FONT_9PT_BLACK_ON_WHITE, (char *) "Are you sure you want to stop");
  displayString(62, 180, FONT_9PT_BLACK_ON_WHITE, (char *) "baking?");
  clearTouchTargets();
  drawTouchButton(60, 230, 160, 72, BUTTON_LARGE_FONT, (char *) "Stop");
  drawTouchButton(260, 230, 160, 105, BUTTON_LARGE_FONT, (char *) "Cancel");
}


// Display the bake temperature and duration on the screen
// This is also a callback routine, called if the user taps in the top-right corner
void displayBakeTemperatureAndDuration(boolean displayCelsius)
{
  // Get the bake duration
  secondsToEnglishString((buffer100Bytes)+50, getBakeSeconds(prefs.bakeDuration));
  // Create a string of the description
  sprintf(buffer100Bytes, "Bake at %d~%c for %s.", displayCelsius? prefs.bakeTemperature: (prefs.bakeTemperature*9/5)+32,
                  displayCelsius? 'C':'F', (buffer100Bytes)+50);
  // Erase the previous description
  tft.fillRect(20, 60, 460, 19, WHITE);
  // Display the new description
  displayString(20, 60, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
}


// Display the countdown timer
void displayBakeSecondsLeft(uint32_t seconds)
{
  static uint16_t oldWidth = 1, timerX = 130;

  // Update the clock display
  uint16_t newWidth = displayString(timerX, 110, FONT_22PT_BLACK_ON_WHITE_FIXED, secondsInClockFormat(buffer100Bytes, seconds));
  // Keep the timer display centered
  if (newWidth != oldWidth) {
    // The width has changed (one less character on the display).  Erase what was there
    tft.fillRect(timerX, 110, newWidth > oldWidth? newWidth : oldWidth, 48, WHITE);
    // Redraw the timer
    oldWidth = newWidth;
    timerX = 240 - (newWidth >> 1);
    displayString(timerX, 110, FONT_22PT_BLACK_ON_WHITE_FIXED, buffer100Bytes);
  }
}


// Returns the bake duration, in seconds
uint32_t getBakeSeconds(uint16_t duration)
{
  uint32_t minutes;

  // Sanity check on the parameter
  if (duration > BAKE_MAX_DURATION)
    return 5;
  
  // 5 to 30 minutes, at 1 minute increments
  if (duration <= 25)
    minutes = duration + 5;

  // 30 minutes to 2 hours at 5 minute increments
  else if (duration <= 43)
    minutes = (duration - 25) * 5 + 30;

  // 2 to 5 hours in 15 minute increments
  else if (duration <= 55)
    minutes = (duration - 43) * 15 + 120;

  // 5 hours to 12 hours in 30 minute increments
  else if (duration <= 69)
    minutes = (duration - 55) * 30 + 300;

  // 12 hours to 48 in 1 hour increments
  else if (duration <= 105)
    minutes = (duration - 69) * 60 + 720;

  // 48+ hours in 3 hour increments
  else if (duration <= 121)
    minutes = (duration - 105) * 180 + 2880;

  // 96+ hours in 12 hour increments
  else
    minutes = (duration - 121) * 720 + 5760;
//  SerialUSB.println("Duration=" + String(duration) + "  minutes=" + String(minutes));

  return minutes * 60;
}

