#pragma once

#include "dac.hpp"
#include "ina.hpp"

class Probe {
    private:
        DAC dac;
        INA ina;
        Probe(DAC_Address, INA_Address);
    public:
        static void init();
        static void destroy();

        static Probe* probe;

        void setVoltage(UVoltage);
        UVoltage readVoltage();
        Current readCurrent();

        // de huidige spanning van de DAC
        UVoltage currentVoltageSet;
};