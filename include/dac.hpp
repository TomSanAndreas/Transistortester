#pragma once
// Used DAC: MCP4276A0T-E/CH, MCP4276A1T-E/CH & MCP4276A2T-E/CH
// Datasheet can be found at http://www.farnell.com/datasheets/1669523.pdf
// See page 49

#include "i2c_device.hpp"

#ifdef DEBUG_ACTIVE // these values should only be programmed when the debugging is active
#define MIN_VOLTAGE 10    // 0.01 V
#define MAX_VOLTAGE 4960  // VCC - 0.04
#endif

typedef unsigned short Voltage; // is always between 0 and 65535, max voltage is 4960
                                // because of the limitation, only the lower 12 bits are used

enum class DAC_Address {
    A0 = 0b1100000,
    A1 = 0b1100001,
    A2 = 0b1100010
};

class DAC : public I2C_Device {
    private:
        Voltage currentVoltage = MIN_VOLTAGE;
    public:
        DAC(DAC_Address);
        ~DAC();
        void setVoltage(Voltage voltage);
};