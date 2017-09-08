// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// This is a library for the Maxim MAX31856 thermocouple IC
// http://datasheets.maximintegrated.com/en/ds/MAX31856.pdf


#ifndef CONTROLEO3MAX31856_H
#define CONTROLEO3MAX31856_H

#include "Arduino.h"

// MAX31856 Registers
// Register 0x00: CR0
#define CR0_AUTOMATIC_CONVERSION                0x80
#define CR0_ONE_SHOT                            0x40
#define CR0_OPEN_CIRCUIT_FAULT_TYPE_K           0x10    // Type-K is 10 to 20 Ohms
#define CR0_COLD_JUNCTION_DISABLED              0x08
#define CR0_FAULT_INTERRUPT_MODE                0x04
#define CR0_FAULT_CLEAR                         0x02
#define CR0_NOISE_FILTER_50HZ                   0x01
#define CR0_NOISE_FILTER_60HZ                   0x00
// Register 0x01: CR1
#define CR1_AVERAGE_1_SAMPLE                    0x00
#define CR1_AVERAGE_2_SAMPLES                   0x10
#define CR1_AVERAGE_4_SAMPLES                   0x20
#define CR1_AVERAGE_8_SAMPLES                   0x30
#define CR1_AVERAGE_16_SAMPLES                  0x40
#define CR1_THERMOCOUPLE_TYPE_B                 0x00
#define CR1_THERMOCOUPLE_TYPE_E                 0x01
#define CR1_THERMOCOUPLE_TYPE_J                 0x02
#define CR1_THERMOCOUPLE_TYPE_K                 0x03
#define CR1_THERMOCOUPLE_TYPE_N                 0x04
#define CR1_THERMOCOUPLE_TYPE_R                 0x05
#define CR1_THERMOCOUPLE_TYPE_S                 0x06
#define CR1_THERMOCOUPLE_TYPE_T                 0x07
#define CR1_VOLTAGE_MODE_GAIN_8                 0x08
#define CR1_VOLTAGE_MODE_GAIN_32                0x0C
// Register 0x02: MASK
#define MASK_COLD_JUNCTION_HIGH_FAULT           0x20
#define MASK_COLD_JUNCTION_LOW_FAULT            0x10
#define MASK_THERMOCOUPLE_HIGH_FAULT            0x08
#define MASK_THERMOCOUPLE_LOW_FAULT             0x04
#define MASK_VOLTAGE_UNDER_OVER_FAULT           0x02
#define MASK_THERMOCOUPLE_OPEN_FAULT            0x01
// Register 0x0F: SR
#define SR_FAULT_COLD_JUNCTION_OUT_OF_RANGE     0x80
#define SR_FAULT_THERMOCOUPLE_OUT_OF_RANGE      0x40
#define SR_FAULT_COLD_JUNCTION_HIGH             0x20
#define SR_FAULT_COLD_JUNCTION_LOW              0x10
#define SR_FAULT_THERMOCOUPLE_HIGH              0x08
#define SR_FAULT_THERMOCOUPLE_LOW               0x04
#define SR_FAULT_UNDER_OVER_VOLTAGE             0x02
#define SR_FAULT_OPEN                           0x01

// Set/clear MSB of first byte sent to indicate write or read
#define READ_OPERATION(x)                       (x & 0x7F)
#define WRITE_OPERATION(x)                      (x | 0x80)

// Register numbers
#define REGISTER_CR0                            0
#define REGISTER_CR1                            1
#define REGISTER_MASK                           2
#define NUM_REGISTERS                           12      // (read/write registers)

// Errors
#define	FAULT_OPEN                              10000   // No thermocouple
#define	FAULT_VOLTAGE                           10001   // Under/over voltage error.  Wrong thermocouple type?
#define NO_MAX31856                             10002   // MAX31856 not communicating or not connected
#define IS_MAX31856_ERROR(x)                    (x >= FAULT_OPEN && x <= NO_MAX31856)

#define CELSIUS                                 0
#define FAHRENHEIT                              1

// Pins used to connect to the MAX31856 IC
#define THERMOCOUPLE_SDI                        6
#define THERMOCOUPLE_SDO                        7
#define THERMOCOUPLE_CS                         21 // SCL
#define THERMOCOUPLE_CLK                        20 // SDA


class	Controleo3MAX31856
{
public:
    void begin(void);
    void writeRegister(byte, byte);
    double readThermocouple(byte unit);
    double readJunction(byte unit);

private:
    long readData();
    void writeByte(byte);
    double verifyMAX31856();
    byte _registers[NUM_REGISTERS];      // Shadow registers.  Registers can be restored if power to MAX31855 is lost
};

#endif  // CONTROLEO3MAX31856_H
