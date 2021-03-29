#pragma once
// Used DAC: MCP4276A0T-E/CH, MCP4276A1T-E/CH & MCP4276A2T-E/CH
// Datasheet can be found at http://www.farnell.com/datasheets/1669523.pdf
// See page 49

#include "i2c_device.hpp"

#ifdef DEBUG_ACTIVE // these values should only be programmed when the debugging is active
#define MIN_VOLTAGE 10    // 0.01 V
#define MAX_VOLTAGE 4960  // VCC - 0.04
#endif

class Probe;

enum class DAC_Address {
    A0 = 0b1100000,
    A1 = 0b1100001,
    A2 = 0b1100010
};

class DAC : public I2C_Device {
    private:
        #ifdef DEBUG_ACTIVE
        UVoltage currentVoltage = MIN_VOLTAGE;
        #else
        UVoltage currentVoltage = 0;
        #endif
        DAC(DAC_Address);

        friend class Probe;
    public:
        ~DAC();
        // zet een rawValue als uitgangsspanning, dit is een waarde tussen 0-4095 (12-bit) die schaalt tussen 0 tot VCC
        void setVoltage(UVoltage rawValue);
        // zet de DAC op power-down mode, waardoor deze de grond verbind met een weerstand van ongeveer 500K
        void turnOff();
        // zet de DAC aan, zodat setVoltage() correct werkt
        void turnOn();
};