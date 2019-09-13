// Written by Peter Easton
// Released under the MIT license
// Build a reflow oven: https://whizoo.com


#define LEARNING_PHASE_INITIAL_RAMP         0
#define LEARNING_PHASE_CONSTANT_TEMP        1
#define LEARNING_PHASE_THERMAL_INERTIA      2
#define LEARNING_PHASE_INSULATION           3
#define LEARNING_PHASE_START_COOLING        4
#define LEARNING_PHASE_COOLING              5
#define LEARNING_PHASE_DONE                 6
#define LEARNING_PHASE_ABORT                7

#define LEARNING_SOAK_TEMP                120  // Target temperature to run tests against
#define LEARNING_INERTIA_TEMP             150  // Target temperature to run inertia tests against

#define LEARNING_RAMP_TO_TEMP_DURATION    200  // Duration of the initial "get-to-temperature" phase
#define LEARNING_CONSTANT_TEMP_DURATION   780  // Duration of the constant temperature measurement
#define LEARNING_INERTIA_DURATION         120  // Duration of the rate-of-rise temperature measurement
#define LEARNING_COOLING_DURATION         400  // Duration of cool-down (insulation) measurement

#define NO_PERFORMANCE_INDICATOR          101  // Don't draw an indicator on the performance bar

// Stay in this function until learning is done or canceled
void learn() {
  uint32_t lastLoopTime = millis();
  uint16_t secondsLeftOfLearning, secondsLeftOfPhase, secondsIntoPhase = 0;
  uint8_t counter = 0;
  uint8_t learningPhase = LEARNING_PHASE_INITIAL_RAMP;
  double currentTemperature = 0;
  uint8_t elementDutyCounter[NUMBER_OF_OUTPUTS];
  boolean isOneSecondInterval = false;
  uint16_t iconsX, i;
  uint8_t learningDutyCycle, learningIntegral = 0, coolingDuration = 0;
  boolean isHeating = true;
  long lastOverTempTime = 0;
  boolean abortDialogIsOnScreen = false;
  
  // Verify the outputs are configured
  if (areOutputsConfigured() == false) {
    showHelp(HELP_OUTPUTS_NOT_CONFIGURED);
    return;
  }

  // Initialize varaibles used for learning
  secondsLeftOfLearning = LEARNING_RAMP_TO_TEMP_DURATION + LEARNING_CONSTANT_TEMP_DURATION + LEARNING_INERTIA_DURATION + LEARNING_COOLING_DURATION;
  secondsLeftOfPhase = LEARNING_RAMP_TO_TEMP_DURATION + LEARNING_CONSTANT_TEMP_DURATION;
  
  // Start with a duty cycle appropriate to the testing temperature
  learningDutyCycle = 60;

  // Calculate the centered position of the heating and fan icons (icons are 32x32)
  iconsX = 240 - (numOutputsConfigured() * 20) + 4;  // (2*20) - 32 = 8.  8/2 = 4

  // Turn on any convection fans
  setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_ON, COOLING_FAN_OFF);

  // Stagger the element start cycle to avoid abrupt changes in current draw
  // Simple method: there are 6 outputs but the first ones are likely the heating elements
  for (i=0; i< NUMBER_OF_OUTPUTS; i++)
    elementDutyCounter[i] = (65 * i) % 100;

  // Set up the screen in preparation for learning
  // Erase the bottom part of the screen
  tft.fillRect(0, 45, 480, 270, WHITE);

  // Display the static strings
  displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Learning has started.  First, figure out");
  displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "the power required to maintain 120~C.");
  SerialUSB.println("Learning started.  Figure out power needed to maintain 120C");

  // Ug, hate goto's!  But this saves a lot of extraneous code.
userChangedMindAboutAborting:

  // Setup the tap targets on this screen
  clearTouchTargets();
  drawButton(110, 230, 260, 87, BUTTON_LARGE_FONT, (char *) "STOP");
  defineTouchArea(20, 150, 440, 170); // Large tap target to stop learning

  // Draw the status bar
  drawPerformanceBar(true, NO_PERFORMANCE_INDICATOR);
  displayString(133, 200, FONT_9PT_BLACK_ON_WHITE, (char *) "Phase Timer: ");

  // Debounce any taps that took us to this screen
  debounce();

  // Keep looping until learning is done
  while (1) {
    // Has there been a touch?
    switch (getTap(CHECK_FOR_TAP_THEN_EXIT)) {
      case 0: 
        // If learning is done (or user taps "stop" in Abort dialog) then clean up and return to the main menu
        if (learningPhase >= LEARNING_PHASE_COOLING || abortDialogIsOnScreen) {
          learningPhase = LEARNING_PHASE_ABORT;
          // Make sure we exit this screen as soon as possible
          lastLoopTime = millis() - 20;
          counter = 40;
        }
        else {
          // User tapped to abort learning
          drawLearningAbortDialog();
          abortDialogIsOnScreen = true;
        }
        break;

      default:
        // The user didn't tap the screen, but if the Abort dialog is up and the phase makes
        // it irrelevant then automatically dismiss it now
        // You never know, maybe the cat tapped "Stop" ...
        if (!abortDialogIsOnScreen || learningPhase < LEARNING_PHASE_COOLING)
          break;
        // Intentional fall-through (simulate user tapped Cancel) ...

      case 1:
        // This is the cancel button of the Abort dialog.  User wants to continue
        // Erase the Abort dialog
        tft.fillRect(0, 110, 480, 210, WHITE);
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
    if (counter == 0 && !abortDialogIsOnScreen && learningPhase <= LEARNING_PHASE_INSULATION)
      displaySecondsLeft(secondsLeftOfLearning, secondsLeftOfPhase);
    // Update the temperature
    if (counter == 2)
      displayTemperatureInHeader();
    // Dump data to the debugging port
    if (counter == 5 && learningPhase != LEARNING_PHASE_DONE)
      DumpDataToUSB(secondsLeftOfLearning, currentTemperature, learningDutyCycle, learningIntegral);

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
    
      // Abort learning
      SerialUSB.println("Thermocouple error:" + String(buffer100Bytes));
      SerialUSB.println("Learning aborted because of thermocouple error!");
      // Show the error on the screen
      drawThickRectangle(0, 90, 480, 230, 15, RED);
      tft.fillRect(30, 105, 420, 115, WHITE);
      displayString(114, 110, FONT_12PT_BLACK_ON_WHITE, (char *) "Learning Error!");
      displayString(40, 150, FONT_9PT_BLACK_ON_WHITE, (char *) "Thermocouple error:");
      displayString(40, 180, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
      // Turn everything off
      setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_OFF, COOLING_FAN_OFF);
      animateIcons(iconsX); 
      // Wait for the user to tap the screen
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      learningPhase = LEARNING_PHASE_ABORT;
    }

    // Fail-safe: the oven should never go above 200C during learning.  This code should never execute
    if (currentTemperature > 200.0 && learningPhase < LEARNING_PHASE_START_COOLING) {
      tft.fillRect(10, LINE(0), 465, 60, WHITE);
      displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Learning Error!");
      displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "Oven exceeded 200~C");
      SerialUSB.println("Learning aborted because oven exceeded 200C!");
      // Start cooling the oven
      learningPhase = LEARNING_PHASE_START_COOLING;
    }

    switch (learningPhase) {
      case LEARNING_PHASE_INITIAL_RAMP:
        // Make changes every second
        if (!isOneSecondInterval)
          break;

        secondsLeftOfLearning--;
        secondsLeftOfPhase--;
        // Is the oven close to the desired temperature?
        i = (uint16_t) LEARNING_SOAK_TEMP - currentTemperature;
        // Reduce the duty cycle as the oven closes in on the desired temperature
        if (i < 30 && i > 15)
          learningDutyCycle = 30;
        if (i < 15) {
          learningPhase = LEARNING_PHASE_CONSTANT_TEMP;
          SerialUSB.println("Initial temperature ramp complete.  Maintaining 120C using all elements");
          learningDutyCycle = 15;
          secondsIntoPhase = 0;
        }
        // Can't stay in the "ramp to temperature" phase too long
       if (secondsLeftOfPhase < LEARNING_CONSTANT_TEMP_DURATION) {
          tft.fillRect(10, LINE(0), 465, 60, WHITE);
          drawPerformanceBar(false, 0);
          displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Learning failed!  Unable to heat");
          displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "up oven quickly enough (all elements)");
          SerialUSB.println("Learning aborted because oven could not ramp up to 120C!");
          learningPhase = LEARNING_PHASE_START_COOLING;
        }
        break;
        
      case LEARNING_PHASE_CONSTANT_TEMP:
        // Make changes every second
        if (!isOneSecondInterval)
          break;

        secondsLeftOfLearning--;
        secondsLeftOfPhase--;
        secondsIntoPhase++;
        
        // Give the user some indiction of how the oven is performing
        if (secondsIntoPhase > 20) {
          // (12% = excellent, 35% = awful)
          i = constrain(learningDutyCycle, 12, 35);
          drawPerformanceBar(false, map(i, 12, 35, 100, 0));
        }

        // Is the oven too hot?
        if (currentTemperature > LEARNING_SOAK_TEMP) {
          if (isHeating) {
            isHeating = false;
            // Turn all heating elements off
            setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_ON, COOLING_FAN_OFF);

            // The duty cycle caused the temperature to exceed the bake temperature, so decrease it
            // (but not more than once every 30 seconds)
            if (millis() - lastOverTempTime > (30 * 1000)) {
              lastOverTempTime = millis();
              if (learningDutyCycle > 0)
                learningDutyCycle--;
            }

            // Reset the bake integral, so it will be slow to increase the duty cycle again
            learningIntegral = 0;
            SerialUSB.println("Over-temp. Elements off");
          }
        }
        else {
          // The oven is heating up
          isHeating = true;

          // Increase the bake integral if not close to temperature
          if (LEARNING_SOAK_TEMP - currentTemperature > 1.0)
            learningIntegral++;
          if (LEARNING_SOAK_TEMP - currentTemperature > 5.0)
            learningIntegral++;
          
          // Has the oven been under-temperature for a while?
          if (learningIntegral > 30) {
            learningIntegral = 0;
            // Increase duty cycles
            if (learningDutyCycle < 100)
              learningDutyCycle++;
              SerialUSB.println("Under-temp. Increasing duty cycle");
          }
        }
        
        // Time to end this phase?
        if (secondsLeftOfPhase == 0) {
          // Save the duty cycle needed to maintain this temperature
          prefs.learnedPower = learningDutyCycle;
          // Move to the next phase
          tft.fillRect(10, LINE(0), 465, 60, WHITE);
          drawPerformanceBar(false, NO_PERFORMANCE_INDICATOR);
          
          // Time to measure the thermal intertia now
          displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Measuring how quickly the oven gets");
          displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "to 150~C using all elements at 80%");
          SerialUSB.println("Measuring thermal inertia - how long it takes to get to 150C using all elements at 80%");
          learningPhase = LEARNING_PHASE_THERMAL_INERTIA;
          secondsLeftOfPhase = LEARNING_INERTIA_DURATION;
          prefs.learnedInertia = 0;
          
          // Crank up the power to 80% to see the rate-of-rise
          learningDutyCycle = 80;
          isHeating = true;
        }
        break;

      case LEARNING_PHASE_THERMAL_INERTIA:
        // Make changes every second
        if (!isOneSecondInterval)
          break;

        secondsLeftOfLearning--;
        secondsLeftOfPhase--;
        prefs.learnedInertia++;

        // Display the intertia score (35 seconds = excellent, 60 seconds = awful)
        // This will start "excellent" and worsen as time ticks by
        if (prefs.learnedInertia > 20) {
          i = constrain(prefs.learnedInertia, 36, 66);
          drawPerformanceBar(false, map(i, 36, 66, 100, 0));
        }
         
        // Has the temperature passed 150C yet?
        if (currentTemperature >= LEARNING_INERTIA_TEMP) {
          // Stop heating the oven
          isHeating = false;
          // Turn all heating elements off
          setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_ON, COOLING_FAN_OFF);

          // Update the message to show the oven is trying to stabilize around 120C again
          tft.fillRect(10, LINE(0), 465, 60, WHITE);
          displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Cooling oven back to 120~C and");
          displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "measuring heat retention ...");
          SerialUSB.println("Cooling back to 120C with all elements off.  Measuring heat retention");

          // Move to the next phase
          drawPerformanceBar(false, NO_PERFORMANCE_INDICATOR);
          learningPhase = LEARNING_PHASE_INSULATION;
          secondsLeftOfLearning = LEARNING_COOLING_DURATION;
          secondsLeftOfPhase = LEARNING_COOLING_DURATION;
          prefs.learnedInsulation = 0;
        }
        
        // Can't stay in the "ramp to temperature" phase too long
       if (secondsLeftOfPhase == 0) {
          tft.fillRect(10, LINE(0), 465, 60, WHITE);
          drawPerformanceBar(false, 0);
          displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Learning failed!  Unable to ramp");
          displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "up oven quickly enough to 150C");
          SerialUSB.println("Learning aborted because oven could not ramp up to 150C!");
          learningPhase = LEARNING_PHASE_START_COOLING;
        }
        break;

      case LEARNING_PHASE_INSULATION:
        // Make changes every second
        if (!isOneSecondInterval)
          break;

        secondsLeftOfLearning--;
        secondsLeftOfPhase--;

        // Is the cool-down time being measured?
        if (currentTemperature >= LEARNING_SOAK_TEMP && currentTemperature <= LEARNING_INERTIA_TEMP) {
          prefs.learnedInsulation++;
          // Display the performance graph if it's been more then 60 seconds of cooling
          if (prefs.learnedInsulation > 60) {
            i = constrain(prefs.learnedInsulation, 80, 150);
            drawPerformanceBar(false, map(i, 80, 150, 0, 100));
          }
        }

        // A very well insulated oven could take longer than the allocated time
        if (!secondsLeftOfLearning) {
          // What happens if a relay gets stuck on?
          if (prefs.learnedInsulation == 0) {
            // The oven is still above 150C!!!
            learningPhase = LEARNING_PHASE_START_COOLING;
            SerialUSB.println("Learning aborted because temperature wasn't going down!");
          }
            
          // Extend the phase duration
          secondsLeftOfLearning = 60;
          secondsLeftOfPhase = 60;
        }

        // Has the measurement been taken (oven cooled to 120C)?
        // Abort if the insulation score is already beyond excellent
        if (currentTemperature < LEARNING_SOAK_TEMP || prefs.learnedInsulation >= 300) {
          // Erase the screen
          tft.fillRect(10, LINE(0), 465, 60, WHITE);
          tft.fillRect(0, 110, 480, 120, WHITE);
          // Show the results now
          showLearnedNumbers();
          // Done with measuring the oven.  Start cooling
          learningPhase = LEARNING_PHASE_START_COOLING;
          // Save all the learned values now
          prefs.learningComplete = true;
          // Force the writing of Prefs to flash
          writePrefsToFlash();
        }

        break;

      case LEARNING_PHASE_START_COOLING:
        isHeating = false;
      
        // Turn off all elements and turn on the fans
        setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_ON, COOLING_FAN_ON);
     
        // If a servo is attached, use it to open the door over 10 seconds
        setServoPosition(prefs.servoOpenDegrees, 10000);

        // Change the STOP button to DONE
        tft.fillRect(150, 242, 180, 36, WHITE);
        drawButton(110, 230, 260, 93, BUTTON_LARGE_FONT, (char *) "DONE");
        
        // Move to the next phase
        learningPhase = LEARNING_PHASE_COOLING;

        // Cooling should be at least 60 seconds in duration
        coolingDuration = 60;
        break;

      case LEARNING_PHASE_COOLING:
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
          setServoPosition(prefs.servoClosedDegrees, 3000);
          // Stay on this screen and wait for the user tap
          learningPhase = LEARNING_PHASE_DONE;
          // Play a tune to let the user know baking is done
          playTones(TUNE_REFLOW_DONE);
        }
        break;

      case LEARNING_PHASE_DONE:
        // Nothing to do here.  Just waiting for user to tap the screen
        break;

      case LEARNING_PHASE_ABORT:
        SerialUSB.println("Learning is over!");
        // Turn all elements and fans off
        setOvenOutputs(ELEMENTS_OFF, CONVECTION_FAN_OFF, COOLING_FAN_OFF);
        // Close the oven door now, over 3 seconds
        setServoPosition(prefs.servoClosedDegrees, 3000);
        // Undo any learned values that weren't saved (prefs are saved if learning completes successfully)
        getPrefs();
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
            // Restrict the top element's duty cycle to 80% to protect the insulation
            // and reduce IR heating of components
            if (elementDutyCounter[i] == (learningDutyCycle < 80? learningDutyCycle: 80))
              setOutput(i, LOW);
            break;
          case TYPE_BOTTOM_ELEMENT:
            // Turn the output on at 0, and off at the duty cycle value
            if (elementDutyCounter[i] == 0)
              setOutput(i, HIGH);
            if (elementDutyCounter[i] == learningDutyCycle)
              setOutput(i, LOW);
            break;
          
          case TYPE_BOOST_ELEMENT: 
            // Give it half the duty cycle of the other elements
            // Turn the output on at 0, and off at the duty cycle value
            if (elementDutyCounter[i] == 0)
              setOutput(i, HIGH);
            if (elementDutyCounter[i] == learningDutyCycle/2)
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
void DisplayLearningTime(uint16_t duration, float temperature, int duty, int integral) {
  // Write the time and temperature to the serial port, for graphing or analysis on a PC
  uint16_t fraction = ((uint16_t) (temperature * 100)) % 100;
  sprintf(buffer100Bytes, "%u, %d.%02d, %i, %i", duration, (uint16_t) temperature, fraction, duty, integral);
  SerialUSB.println(buffer100Bytes);
}


// Draw the abort dialog on the screen.  The user needs to confirm that they want to exit bake
void drawLearningAbortDialog()
{
  drawThickRectangle(0, 110, 480, 210, 8, RED);
  tft.fillRect(30, 118, 420, 173, WHITE);
  displayString(124, 126, FONT_12PT_BLACK_ON_WHITE, (char *) "Stop Learning");
  displayString(62, 165, FONT_9PT_BLACK_ON_WHITE, (char *) "Are you sure you want to stop");
  displayString(62, 195, FONT_9PT_BLACK_ON_WHITE, (char *) "learning?");
  clearTouchTargets();
  drawTouchButton(60, 240, 160, 72, BUTTON_LARGE_FONT, (char *) "Stop");
  drawTouchButton(260, 240, 160, 105, BUTTON_LARGE_FONT, (char *) "Cancel");
}


// Display the countdown timer
void displaySecondsLeft(uint32_t overallSeconds, uint32_t phaseSeconds)
{
  static uint16_t oldOverallWidth = 1, overallTimerX = 130;

  // Update the overall seconds remaining clock display
  uint16_t newWidth = displayString(overallTimerX, 140, FONT_22PT_BLACK_ON_WHITE_FIXED, secondsInClockFormat(buffer100Bytes, overallSeconds));
  // Keep the timer display centered
  if (newWidth != oldOverallWidth) {
    // The width has changed (one less character on the display).  Erase what was there
    tft.fillRect(overallTimerX, 140, newWidth > oldOverallWidth? newWidth : oldOverallWidth, 48, WHITE);
    // Redraw the timer
    oldOverallWidth = newWidth;
    overallTimerX = 240 - (newWidth >> 1);
    displayString(overallTimerX, 140, FONT_22PT_BLACK_ON_WHITE_FIXED, buffer100Bytes);
  }

  // Update the phase seconds remaining clock display
  displayFixedWidthString(285, 200, secondsInClockFormat(buffer100Bytes, phaseSeconds), 5, FONT_9PT_BLACK_ON_WHITE_FIXED);
}


// Colors for the bar graph
#define PERFORMANCE_BAD               0xF082  // (tft.convertTo16Bit(0xF01111))
#define PERFORMANCE_OKAY              0x09BF  // (tft.convertTo16Bit(0x0D35F9))
#define PERFORMANCE_GOOD              0x5F0B  // (tft.convertTo16Bit(0x5EE05F))

// Draw the performance graph to indicate instantaneous performance
void drawPerformanceBar(boolean redraw, uint8_t percentage)
{
  static uint16_t lastStatusX = 0;

  if (redraw) {
    // The status bar must be redrawn
    // Erase the space that the bar occupies
    tft.fillRect(30, 110, 440, 20, WHITE);
    // Draw the color strips
    tft.fillRect(30, 112, 140, 16, PERFORMANCE_BAD);
    tft.fillRect(170, 112, 140, 16, PERFORMANCE_OKAY);
    tft.fillRect(310, 112, 140, 16, PERFORMANCE_GOOD);
    lastStatusX = 0;
  }
  else {
    // Erase the mark, if there was one
    if (lastStatusX) {
      // Draw white at the top and bottom
      tft.fillRect(lastStatusX-3, 110, 7, 2, WHITE);
      tft.fillRect(lastStatusX-3, 128, 7, 2, WHITE);
      // Restore the color to the bar
      for (uint16_t i= lastStatusX-3; i <= lastStatusX+3; i++)
        tft.fillRect(i, 112, 1, 16, i < 170? PERFORMANCE_BAD: i >= 310? PERFORMANCE_GOOD: PERFORMANCE_OKAY);
    }
  
    // Draw the mark at the new location
    if (percentage <= 100) {
      lastStatusX = map(percentage, 0, 100, 31, 449);
      tft.fillRect(lastStatusX-3, 110, 7, 20, BLACK);
      tft.fillRect(lastStatusX-2, 111, 5, 18, WHITE);
    }
  }
}


// Calculate the overall oven score
uint8_t ovenScore()
{
  uint8_t score;

  // Ability to hold temperature using very little power is worth 40%
  // 12% (or less) duty cycle scores all 40 points, sliding to 0 if required power is 30%
  score = map(constrain(prefs.learnedPower, 12, 30), 12, 30, 40, 0);

  // Thermal inertia is worth another 40%
  // Taking 36 seconds (or less) to gain 30C scores all the points, sliding to 0 if it takes 60 seconds or longer
  score += map(constrain(prefs.learnedInertia, 36, 60), 36, 60, 40, 0);

  // The remaining 20% is allocated towards insulation
  // A well-insulated oven will lose 30C in just over 2 minutes, and a poorly insulated one in 80 seconds or less
  score += map(constrain(prefs.learnedInsulation, 80, 150), 80, 150, 0, 20);

  // And that is the oven score!
  return score;
}


#define GOOD          0
#define OKAY          1
#define BAD           2

// The learned numbers are shown once the oven has completed the 1-hour learning run
void showLearnedNumbers()
{
  uint16_t score, offset, result;
  char *description[] = {"(Good)", "(Okay)", "(Bad)"};
  
  // Display the power required to keep the oven at a stable temperature
  sprintf(buffer100Bytes, "Power: %d%% ", prefs.learnedPower);
  offset = displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes) + 10;
  // Show the emoticon that corresponds to the power
  result = prefs.learnedPower < 18? GOOD : prefs.learnedPower < 24? OKAY : BAD;
  renderBitmap(BITMAP_SMILEY_GOOD + result, offset, LINE(0)-3);
  // Add the width of the emoticon, plus some space
  offset += 45;
  displayString(offset, LINE(0), FONT_9PT_BLACK_ON_WHITE, description[result]);

  // Display the time needed to reach the higher temperatures (inertia)
  sprintf(buffer100Bytes, "Inertia: %ds ", prefs.learnedInertia);
  offset = displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes) + 10;
  // Show the emoticon that corresponds to the inertia
  result = prefs.learnedInertia < 46? GOOD : prefs.learnedInertia < 56? OKAY : BAD;
  renderBitmap(BITMAP_SMILEY_GOOD + result, offset, LINE(1)-3);
  // Add the width of the emoticon, plus some space
  offset += 45;
  displayString(offset, LINE(1), FONT_9PT_BLACK_ON_WHITE, description[result]);

  // Display the insulation score
  sprintf(buffer100Bytes, "Insulation: %ds ", prefs.learnedInsulation);
  offset = displayString(10, LINE(2), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes) + 10;
  // Show the emoticon that corresponds to the insulation
  result = prefs.learnedInsulation > 127? GOOD : prefs.learnedInsulation > 103? OKAY : BAD;
  renderBitmap(BITMAP_SMILEY_GOOD + result, offset, LINE(2)-3);
  // Add the width of the emoticon, plus some space
  offset += 45;
  displayString(offset, LINE(2), FONT_9PT_BLACK_ON_WHITE, description[result]);

  // Display the overall oven score
  displayString(10, LINE(3), FONT_9PT_BLACK_ON_WHITE, (char *) "Oven score: ");
  score = ovenScore();
  sprintf(buffer100Bytes, "%d%% = ", score);
  strcat(buffer100Bytes, score >90? "Excellent": score>80? "Very good": score>70? "Good": score>60? "Marginal": "Not that good");
  displayString(159, LINE(3), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
}

