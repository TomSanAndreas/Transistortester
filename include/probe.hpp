#pragma once

#include "dac.hpp"
#include "ina.hpp"

struct MeasureResult {
    UVoltage usedVoltage;
    UVoltage minV = 32000, avgV, maxV = 0;
    Current minA = 32000, avgA, maxA = 0;
    MeasureResult() {}
};

struct ProbeCombination;

class Probe {
    private:
        DAC dac;
        INA ina;
        Probe(DAC_Address, INA_Address);
        // dit bevat de verhouding tussen een bit en een aantal mV, zodanig dat 4095 (12x '1') gelijk is aan VCC
        double voltBitRatio;
        Voltage vOffset = 0;
        UVoltage currentVoltageBitsSet = 0;
        UVoltage exactVoltageLUT[4096];
        bool isDacTurnedOn = true;
    public:
        static void init();
        static void destroy();

        static Probe* probe;
        static ProbeCombination* combinations;

        void calibrate(void (*progressIndicator)(), double* progress);

        // zet de DAC in Powerdown modus, waardoor deze geen spanning/stroom levert en intern doorverbonden is met een weerstand van ~500K
        void turnOff();

        // stel een *CONSTANTE* offset in op de gemeten spanning
        // BELANGRIJK: gebruik de calibrate()-functie om het volledig effect hiervan te krijgen!
        void setOffset(Voltage);
        // voltage is in mV
        void setVoltage(UVoltage);
        // laat de DAC precies 1 bit toenemen in spanning
        void increaseVoltage();
        // laat de DAC precies 1 bit afnemen in spanning
        void decreaseVoltage();
        // newShunt is in Ohm
        void setShunt(float newShunt);
        // expectedCurrent is in µA
        float adjustShuntUsingCurrent(Current expectedCurrent);
        
        // krijg shuntvoltage in µV
        Voltage readShuntVoltage();
        // lees N samples en bereken het gemiddelde, in µV
        Voltage readAverageShuntVoltage(unsigned int nSamples);
        // voltage is in mV
        UVoltage readVoltage();
        // lees N samples en bereken het gemiddelde, in mV
        UVoltage readAverageVoltage(unsigned int nSamples);
        // current is in µA
        Current readCurrent();
        // lees N samples en bereken het gemiddelde, in µA
        Current readAverageCurrent(unsigned int nSamples);
        
        // krijg een compleet resultaat, na N samples
        MeasureResult doFullMeasure(unsigned int nSamples);

        // de huidige spanning van de DAC
        UVoltage currentVoltageSet;
};

struct ProbeCombination {
    Probe* first,* second,* third;
    unsigned int firstPinNumber, secondPinNumber, thirdPinNumber;
    static ProbeCombination* possibleCombinations;
};