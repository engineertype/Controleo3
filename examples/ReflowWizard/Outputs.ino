// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// There are 6 outputs on Controleo3:
// Outputs 1 through 5
//  - Go to the screw terminals
//  - Control n-channel MOSFETs, which switch 4.5V (5V less schottky diode)
//  - MOSFET's can handle currents up to 240mA
//  - Protected by 240 Ohm resistors, so current is limited to 19mA per output
//  - Output 1 = PA15 (Arduino = D5)
//  - Output 2 = PB30 (Arduino = not available)
//  - Output 3 = PB17 (Arduino = not available)
//  - Output 4 = PB09 (Arduino = A2)
//  - Output 5 = PB08 (Arduino = A1)
// Output 6
//  - Is just a bare pin broken out to the middle of the PCB
//  - 3.3V because it is a microcontroller pin
//  - No input protection and no MOSFET
//  - Can be used as a digital input, if necessary using pinMode(SCK, INPUT) and digitalRead(SCK)
//  - Output 6 = PB11 (Arduino = SCK)
//
//  On the board (and in the build guide) the outputs are 1 through 6. In software they are 0 through 5.


volatile uint32_t *portAOut, *portAMode, *portBOut, *portBMode;
static boolean outputState[NUMBER_OF_OUTPUTS];

// Initialize the registers controlling the outputs, and turn them off
void initOutputs() {
  // Get pointer to the registers
  portAOut   = portOutputRegister(digitalPinToPort(5));
  portAMode  = portModeRegister(digitalPinToPort(5));
  portBOut   = portOutputRegister(digitalPinToPort(A2));
  portBMode  = portModeRegister(digitalPinToPort(A2)); 

  // Set all I/O modes to outputs
  *portAMode |= SETBIT15;
  *portBMode |= (SETBIT08 + SETBIT09 + SETBIT11 + SETBIT17 + SETBIT30);

  // Set all outputs low (turn off relays)
  for (int i=0; i<=NUMBER_OF_OUTPUTS; i++)
    setOutput(i, LOW);
}


// Turn an output on or off
void setOutput(uint8_t outputNumber, boolean state) {
  // Make sure it is a valid output
  if (outputNumber > NUMBER_OF_OUTPUTS) {
    SerialUSB.println("setOutput: outputNumber is not in range!");
    return;
  }

  // Save the new state
  outputState[outputNumber] = state;
  
  switch (outputNumber) {
    case 0: 
      if (state == LOW)
        *portAOut &= CLEARBIT15;
      else
        *portAOut |= SETBIT15; 
      break;
    
    case 1: 
      if (state == LOW)
        *portBOut &= CLEARBIT30;
      else
        *portBOut |= SETBIT30; 
      break;
    
    case 2: 
      if (state == LOW)
        *portBOut &= CLEARBIT17;
      else
        *portBOut |= SETBIT17; 
      break;
    
    case 3: 
      if (state == LOW)
        *portBOut &= CLEARBIT09;
      else
        *portBOut |= SETBIT09; 
      break;
    
    case 4: 
      if (state == LOW)
        *portBOut &= CLEARBIT08;
      else
        *portBOut |= SETBIT08; 
      break;
    
    case 5: 
      if (state == LOW)
        *portBOut &= CLEARBIT11;
      else
        *portBOut |= SETBIT11; 
      break;
  }
}


// Get the current state of an output
boolean getOutput(uint8_t outputNumber)
{
  // Make sure it is a valid output
  if (outputNumber > NUMBER_OF_OUTPUTS) {
    SerialUSB.println("getOutput: outputNumber is not in range!");
    return false;
  }
  return outputState[outputNumber];
}


// Turn elements and fans on or off in one go
void setOvenOutputs(boolean elementsOn, boolean convectionFanOn, boolean coolingFanOn)
{
  for (uint8_t i=0; i< NUMBER_OF_OUTPUTS; i++) {
    switch (prefs.outputType[i]) {
      case TYPE_UNUSED:
        break;
      case TYPE_CONVECTION_FAN:
        setOutput(i, convectionFanOn);
        break;
      case TYPE_COOLING_FAN:
        setOutput(i, coolingFanOn);
        break;
      default:
        // This can only be used to turn the elements off
        if (!elementsOn)
          setOutput(i, LOW);
        break;
    }
  }
}


// Turn the convection fan on or off
void turnConvectionFanOn(boolean on)
{
  for (uint8_t i=0; i< NUMBER_OF_OUTPUTS; i++) {
    if (prefs.outputType[i] == TYPE_CONVECTION_FAN)
      setOutput(i, on);
  }
}


// Turn the cooling fan on or off
void turnCoolingFanOn(boolean on)
{
  for (uint8_t i=0; i< NUMBER_OF_OUTPUTS; i++) {
    if (prefs.outputType[i] == TYPE_COOLING_FAN)
      setOutput(i, on);
  }
}


// Check to see if outputs are configured.  Prevent bake/reflow if not
// At least 2 elements must be heating elements
boolean areOutputsConfigured()
{
  uint8_t numberConfigured = 0;
  for (uint8_t i=0; i< NUMBER_OF_OUTPUTS; i++) {
    if (isHeatingElement(prefs.outputType[i]))
      numberConfigured++;
  }
  if (numberConfigured >=2)
    return true;
  return false;
}


// See how many outputs are used
uint8_t numOutputsConfigured()
{
  uint8_t numberConfigured = 0;
  for (uint8_t i=0; i< NUMBER_OF_OUTPUTS; i++) {
    if (prefs.outputType[i] != TYPE_UNUSED)
      numberConfigured++;
  }
  return numberConfigured;
}

