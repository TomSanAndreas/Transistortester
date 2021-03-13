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
        UVoltage currentVoltage = MIN_VOLTAGE;
        DAC(DAC_Address);

        friend class Probe;
    public:
        ~DAC();
        //TODO: kalibratiewaarde opstellen: 4095 is MAX_VOLTAGE, maar is dan automatisch gelijkgesteld aan de voedingsspanning (ong 5.2V): eerst DAC's op maximum zetten en in de probe via de INA de spanning nameten om zo een correcte schaal te gebruiken
        void setVoltage(UVoltage voltage);
};