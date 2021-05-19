#pragma once
// Gebruikte DAC: MCP4276A0T-E/CH, MCP4276A1T-E/CH & MCP4276A2T-E/CH
// Datasheet is te vinden op http://www.farnell.com/datasheets/1669523.pdf
// Zie pagina 49

#include "i2c_device.hpp"

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