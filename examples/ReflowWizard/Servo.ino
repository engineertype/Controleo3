// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//

// Timer TC3 is used for 2 things:
// 1. Take thermocouple readings every 200ms (5 times per second)
// 2. Control the servo used to open the oven door
//
// Servo timer interrupt operation
// ===============================
// See http://en.wikipedia.org/wiki/Servo_control for more information regarding servo operation.
// Note: Some (most) servos only rotate around 90°, not the 180° that is possible in theory.
// The servo position information should be sent every 20ms, or 50 times per second.  To do this:
//   - Timer TC3 is set up with 2 counters
//     1. CC0 is configured to fire every 20ms (50 times per second)
//     2. CC1 is configured as the end-of-servo-pulse, to cut the signal pulse to the servo
//
// For every 10 times CC0 fires, a call is made to get a thermocouple reading.  If servo movement
// is enabled (servoMovements is non-zero) then the servo pin is set high.  It must be lowered
// somewhere between 1ms and 2ms later, depending on the desired position.  To do this, the
// appropriate value is written to CC[1].reg.  Keep in mind that unlike CC0, CC1 does not reset
// Timer TC3's counter.  The steps look something like this:
//   a. Counter = 0: Write servo pin HIGH, and set CC1 to correct duration.
//   b. Counter = CC[1].reg: Write servo pin low
//   c. Counter = CC[0].reg: Counter is set back to 0 (go to a.)
// Once the servo has reached the desired position, servo movement stops because servoMovements becomes zero.
//
// Plan B:
// =======
// The above scheme souunds great, but CC1 was delayed by up to 1ms every now & then.  This resulted in very erratic 
// servo behavior.  The work-around now is to wait inside the ISR for the duration of the servo pulse.  This is
// an awful thing to do; ISR's should always do the absolute minimum and then return.  However, servo movement
// is important and the delay is 2ms at most.  Nothing should be affected by this delay.  The touchscreen has
// a dedicated controller so it won't be affected.

// With a 48MHz clock, the prescaler is set to 64.  This gives a timer speed of 48,000,000 / 64 = 750,000. This means
// the timer counts from 0 to 750,000 in one second.  We'd like the interrupt to fire 50 times per second so we set
// the compare register CC[0].reg to 750,000 / 50 = 15,000.


#define MIN_PULSE_WIDTH        800     // The shortest pulse (nanoseconds) sent to a servo (Arduino's servo library has 544)
#define MAX_PULSE_WIDTH        2200    // The longest pulse (nanoseconds) sent to a servo (Arduino's servo library has 2400)

// 15,000 counter => 20ms (20,000 ns).  So converting pulse to a counter value ...
#define MIN_PULSE_COUNTER      ((MIN_PULSE_WIDTH * 3) / 4)   // 600
#define MAX_PULSE_COUNTER      ((MAX_PULSE_WIDTH * 3) / 4)   // 1650

#define SERVO_PIN_HIGH         *portBOut |= SETBIT31;
#define SERVO_PIN_LOW          *portBOut &= CLEARBIT31;
 
// Variables used to control servo movement
volatile uint16_t servoMovements;          // Number of movements (pulses) to reach the desired position
volatile int16_t servoIncrement;           // The amount to increase/decrease the pulse every interrupt


// Starts the timer.  Called on startup
void initializeTimer() {
  // Enable clock for TC 
  REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3);
  while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // Wait for sync 

  // Get timer struct 
  TcCount16* TC = (TcCount16*) TC3;

  // Disable TC and wait for sync
  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1);

  // Set Timer counter Mode to 16 bits and wait for sync
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;  
  while (TC->STATUS.bit.SYNCBUSY == 1);

  // Set TC as normal Normal Frq and wait for sync 
  TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_NFRQ; 
  while (TC->STATUS.bit.SYNCBUSY == 1);

  // Set prescaler and wait for sync
  TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV64;   
  while (TC->STATUS.bit.SYNCBUSY == 1);

  // Timer timer duration to 20ms and wait for sync
  TC->CC[0].reg = 15000;
  while (TC->STATUS.bit.SYNCBUSY == 1);
  
  // Interrupts 
  TC->INTENSET.reg = 0;              // Disable all interrupts
  TC->INTENSET.bit.MC0 = 1;          // Enable compare match to CC0 (20ms timer)
//  TC->INTENSET.bit.MC1 = 1;          // Enable compare match to CC1 (servo pulse timer)

  // Enable InterruptVector
  NVIC_EnableIRQ(TC3_IRQn);

  // Enable TC and wait for sync
  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); 

  // Set servo pin as output
  *portBMode |= SETBIT31;
  SERVO_PIN_LOW;

  // Assume the servo is in the closed position
  TC->CC[1].reg = degreesToTimerCounter(prefs.servoClosedDegrees) + 1;
}


// Interrupt handler for TC3
void TC3_Handler()
{
  volatile static int thermocoupleTimer = 0;

  TcCount16* TC = (TcCount16*) TC3; 

  // Did a compare to CC0 cause the interrupt?
  if (TC->INTFLAG.bit.MC0 == 1) {
    // This interrupt fires every 20ms (50 times per second)
    // Reset the counter to zero immediately
    TC->COUNT.reg = 0;

    // Has the servo reached the desired position?
    if (servoMovements) {
      // Force the counter to zero
      while (TC->STATUS.bit.SYNCBUSY == 1);
      
      // Write the servo signal pin high.  Ideally we'd lower this when CC1 fires, but
      // the timing (and hence servo movement) was erratic, so waiting inside this ISR. Not
      // a good way to do it at all - but it is a maximum of 2ms.  The touchscreen has its own
      // IC so nothing is adversely affected by this wait.
      SERVO_PIN_HIGH;
      while (TC->COUNT.reg < TC->CC[1].reg) ;
      SERVO_PIN_LOW;
      
      // Move the servo to the next position
      TC->CC[1].reg += servoIncrement;
      servoMovements--;

      // Send a few more pulses to hold this position if at the end of the motion
      if (!servoMovements && servoIncrement) {
        servoMovements = 25;
        servoIncrement = 0;
      }
    }

    // Read the thermocouple 5 times per second (every 0.2 seconds)
    if (++thermocoupleTimer >= 10) {
      thermocoupleTimer = 0;
      takeCurrentThermocoupleReading();
    }

    // Clear the interrupt flag
    TC->INTFLAG.bit.MC0 = 1;
  }
  
  // Did a compare to CC1 cause the interrupt?
  // This is going to fire every 20ms, even if servo movement isn't needed.  Yes,
  // it could be made more efficient, but code simplicity wins!
  if (TC->INTFLAG.bit.MC1 == 1) {
    // Ideally this is where the servo signal would be lowered - but the timing was so erratic
    // Clear the interrupt flag
    TC->INTFLAG.bit.MC1 = 1;    
  }
}


// Move the servo to servoDegrees, in timeToTake milliseconds (1/1000 second)
void setServoPosition(uint8_t servoDegrees, uint16_t timeToTake) {
  TcCount16* TC = (TcCount16*) TC3;     // Get timer struct
  sprintf(buffer100Bytes, "Servo: move to %d degrees, over %d ms", servoDegrees, timeToTake);
  SerialUSB.println(buffer100Bytes);
  // Make sure the degrees are 0 - 180
  if (servoDegrees > 180)
    return;

  // Disable TC and wait for sync
  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1);
  
  // Figure out what the end timer value should be
  uint16_t servoEndValue = degreesToTimerCounter(servoDegrees);
  
  // If the servo is already in this position, then don't do anything
  if (servoEndValue == TC->CC[1].reg)
    goto reenable;
    
  // Figure out how many movements this will take (a movement is made every 20ms)
  servoMovements = timeToTake / 20;

  if (servoEndValue - TC->CC[1].reg > 0) {
    // Increment must be positive
    servoIncrement = ((servoEndValue - TC->CC[1].reg) / servoMovements) + 1;
    // There are rounding errors, so calculate the number of movements again
    servoMovements = (servoEndValue - TC->CC[1].reg) / servoIncrement;
  }
  else {
    // Increment must be negative
    servoIncrement = -1 + ((servoEndValue - TC->CC[1].reg) / servoMovements);
    // There are rounding errors, so calculate the number of movements again
    servoMovements = (servoEndValue - TC->CC[1].reg) / servoIncrement;    
  }

reenable:
  // Enable TC and wait for sync
  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); 
} 


// Convert degrees (0-180) to a timer counter value
uint16_t degreesToTimerCounter(uint8_t servoDegrees) {
  // Get the pulse duration in microseconds
  return map(servoDegrees, 0, 180, MIN_PULSE_COUNTER, MAX_PULSE_COUNTER);
}

