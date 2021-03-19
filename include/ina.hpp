#pragma once
// Used current sensor: INA233
// Datasheet can be found at https://www.ti.com/lit/ds/symlink/ina233.pdf
// See page 15

#include "i2c_device.hpp"

// // for calculation, see page 16
// // max current is 1 here
// #define CAL 32768 * 0.00512 / R_SHUNT

// register maps
#define MFR_READ_VSHUNT 0xD1
#define READ_VIN        0x88
#define READ_IIN        0x89

class Probe;

enum class INA_Address {
    INA1 = 0b1000001,
    INA2 = 0b1000100,
    INA3 = 0b1000000
};

class INA : public I2C_Device {
    private:
        INA(INA_Address);
        friend class Probe;
    public:
        float rShunt = 4.0;
        // result is signed and in µA
        Current readCurrent();
        // result is unsigned and in mV
        UVoltage readVoltage();
        // result is signed and in µV
        Voltage readShuntVoltage();
};