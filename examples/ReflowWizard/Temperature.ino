// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//

#define CR0_INIT  (CR0_AUTOMATIC_CONVERSION + CR0_OPEN_CIRCUIT_FAULT_TYPE_K /* + CR0_NOISE_FILTER_50HZ */)
#define CR1_INIT  (CR1_AVERAGE_2_SAMPLES + CR1_THERMOCOUPLE_TYPE_K)
#define MASK_INIT (~(MASK_VOLTAGE_UNDER_OVER_FAULT + MASK_THERMOCOUPLE_OPEN_FAULT))

extern Controleo3MAX31856 thermocouple;


// Instead of getting instantaneous readings from the thermocouple, get an average.
// Also, some convection ovens have noisy fans that generate spurious short-to-ground and 
// short-to-vcc errors.  This will help to eliminate those.
// takeCurrentThermocoupleReading() is called from the Timer 1 interrupt (see "Servo" tab).  It is
// called 5 times per second.

#define NUM_READINGS           15  // Number of readings to average the temperature over (5 readings = 1 second)
#define ERROR_THRESHOLD        5   // Number of consecutive faults before a fault is returned

volatile float MAX31856temperature;

// Initialize the MAX31856's registers
void initTemperature() {
  // Initializing the MAX31855's registers
  thermocouple.writeRegister(REGISTER_CR0, CR0_INIT + prefs.lineVoltageFrequency);
  thermocouple.writeRegister(REGISTER_CR1, CR1_INIT);
  thermocouple.writeRegister(REGISTER_MASK, MASK_INIT);
}


// This function is called every 200ms from the Timer 1 (servo) interrupt
void takeCurrentThermocoupleReading()
{
  volatile static int readingNum = 0;
  volatile static float recentTemperatures[NUM_READINGS];
  volatile static int temperatureErrorCount = 0;
    
  // The timer has fired.  It has been 0.2 seconds since the previous reading was taken
  // Take a thermocouple reading
  float temperature = thermocouple.readThermocouple(CELSIUS);
  
  // Is there an error?
  if (IS_MAX31856_ERROR(temperature)) {
    // Noise can cause spurious short faults.  These are typically caused by the convection fan
    if (temperatureErrorCount < ERROR_THRESHOLD)
      temperatureErrorCount++;
    else
      MAX31856temperature = temperature;
  }
  else {
    // There is no error.  Save the temperature
    recentTemperatures[readingNum] = temperature;
    readingNum = (readingNum + 1) % NUM_READINGS;

    // Calculate the average over the readings
    temperature = 0;
    for (int i=0; i< NUM_READINGS; i++)
      temperature += recentTemperatures[i];
    MAX31856temperature = temperature / NUM_READINGS;
    
    // Clear any previous error
    temperatureErrorCount = 0;
  }
}

//#define SIMULATE_TEMPERATURE
#ifdef SIMULATE_TEMPERATURE
#define NUM_READINGS  15
float getCurrentTemperature() {
  static float temperature = 24.34;
  static uint8_t outputValue[NUM_READINGS];
  static uint32_t lastUpdate = 0;

  if (millis() - lastUpdate < 1)  // Was 900
    return temperature;
  lastUpdate = millis();

  // Shift the earlier readings
  for (uint8_t i=NUM_READINGS-1; i>0; i--)
    outputValue[i] = outputValue[i-1];
  outputValue[0] = 0;

  // Get the current readings
  for (uint8_t i=0; i<NUMBER_OF_OUTPUTS; i++) {
    if (isHeatingElement(prefs.outputType[i]) && getOutput(i) == HIGH)
      outputValue[0]++;
  }

  // Get the sum over the previous readings
  float sum = 0;
  for (uint8_t i=0; i< NUM_READINGS; i++)
    sum += outputValue[i];

  // If no elements are on, the temperature will drop
  if (temperature > 25.0)
    temperature -= ((temperature - 25.0) / 80); 

  // If elements are on the temperature will increase
  temperature += sum / 7;  // Was 12

    // Return the temperature
  return temperature;
}
#else
// Routine used by the main app to get temperatures
float getCurrentTemperature() {
  static float temperature = 0;
  static uint32_t lastUpdate = 0;
  float temperature2;

  // Don't take the reading too often
  if (millis() - lastUpdate < 200)
    return temperature;
  lastUpdate = millis();

  // The temperature might be updated by the ISR while reading the value.  Take
  // the reading twice to make sure the right value was obtained.
  // Now that "Plan B" is in place (see Servo) we could disable interrupts while
  // taking a reading - but this works so why change it?
  do {
    temperature = MAX31856temperature;
    temperature2 = MAX31856temperature;
  } while (temperature != temperature2);

  // Return the temperature
  return temperature;
}
#endif


// Convert the temperature to a string
char *getTemperatureString(char *str, float temperature, boolean displayInCelsius)
{
  if (!IS_MAX31856_ERROR(temperature)) {
    if (!displayInCelsius)
      temperature = (temperature * 9 / 5) + 32;
    // Negative temperatures are not handled (they'll be displayed as "0.0C")
    uint16_t number = (uint16_t) temperature;
    uint16_t decimal = (uint16_t) ((temperature - number) * 100);
    sprintf(str, "%d.%02d~%c", number, decimal, displayInCelsius? 'C':'F');
  }
  else {
    // Don't display the temperature
    sprintf(str, "---~%c", displayInCelsius? 'C':'F');
  }
  return str;
}

