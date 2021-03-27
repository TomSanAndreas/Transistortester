#pragma once

#include "dac.hpp"
#include "ina.hpp"

struct MeasureResult {
    UVoltage usedVoltage;
    UVoltage minV = 32000, avgV, maxV = 0;
    Current minA = 32000, avgA, maxA = 0;
    MeasureResult() {}
};

class Probe {
    private:
        DAC dac;
        INA ina;
        Probe(DAC_Address, INA_Address);
        // this contains the ratio between a bit and x mV, so that 4095 equals VCC
        double voltBitRatio;
        Voltage vOffset = 0;
        UVoltage currentVoltageBitsSet = 0;
        UVoltage exactVoltageLUT[4096];
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
        
        // get shunt voltage in µV
        Voltage readShuntVoltage();
        // read N samples and take average, in µV
        Voltage readAverageShuntVoltage(unsigned int nSamples);
        // voltage is in mV
        UVoltage readVoltage();
        // read N samples and take average, in mV
        UVoltage readAverageVoltage(unsigned int nSamples);
        // current is in µA
        Current readCurrent();
        // read N samples and take average, in µA
        Current readAverageCurrent(unsigned int nSamples);
        
        // get a detailed result, with data from N samples
        MeasureResult doFullMeasure(unsigned int nSamples);

        // de huidige spanning van de DAC
        UVoltage currentVoltageSet;
};