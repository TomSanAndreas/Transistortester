#include "dac.hpp"

DAC::DAC(DAC_Address addr) : I2C_Device((byte) addr) {
    // set GAIN, low startvoltage ... and other properties
    //queue(...);
    //...
    //send();
}

void DAC::setVoltage(Voltage voltage) {
    #ifdef DEBUG_ACTIVE
    if (voltage > MAX_VOLTAGE || voltage < MIN_VOLTAGE) {
        std::cout << "Invalid voltage: " << voltage << "mV.\n";
    }
    #endif
    currentVoltage = voltage;
}