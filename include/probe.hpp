#pragma once

#include "dac.hpp"
#include "ina.hpp"

class Probe {
    private:
        DAC dac;
        INA ina;
        Probe(DAC_Address, INA_Address);
    public:
        static Probe first, second, third;

        void setVoltage(UVoltage);
        UVoltage readVoltage();
        Current readCurrent();
};