#pragma once

#include "dac.hpp"
#include "ina.hpp"

class Probe {
    private:
        DAC dac;
        INA ina;
        Probe(DAC_Address, INA_Address);
        // this contains the ratio between a bit and x mV, so that 4095 equals VCC
        double voltBitRatio;
        Voltage vOffset = 0;
    public:
        static void init();
        static void destroy();

        static Probe* probe;

        void calibrate();

        // set a constant offset on the measured voltage in mV
        // IMPORTANT: use calibrate() to see the full effect of the changes!
        void setOffset(Voltage);
        // voltage is in mV
        void setVoltage(UVoltage);
        // newShunt is in Ohm
        void setShunt(float newShunt);
        // expectedCurrent is in µA
        float adjustShuntUsingCurrent(Current expectedCurrent);

        // voltage is in mV
        UVoltage readVoltage();
        // current is in µA
        Current readCurrent();

        // de huidige spanning van de DAC
        UVoltage currentVoltageSet;
};