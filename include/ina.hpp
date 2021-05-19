#pragma once
// Gebruikte "stroomsensor": INA233
// Datasheet is te vinden op https://www.ti.com/lit/ds/symlink/ina233.pdf
// Zie pagina 15

#include "i2c_device.hpp"

// gebruikte registers
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
        // resultaat is signed en is uitgedrukt in µA
        Current readCurrent();
        // resultaat is unsigned en is uitgedrukt in mV
        UVoltage readVoltage();
        // resultaat is signed en is uitgedrukt in µV
        Voltage readShuntVoltage();
};