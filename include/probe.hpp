#pragma once

#include "dac.hpp"
#include "ina.hpp"

class Probe;

struct ProbeComponenten {
    DAC_Address dac;
    INA_Address ina;
    private:
        ProbeComponenten() {}
        ProbeComponenten(DAC_Address d, INA_Address i) : dac(d), ina(i) {}
        friend class Probe;
};

class Probe {
    private:
        DAC dac;
        INA ina;
    public:
        Probe(const ProbeComponenten&);
        void setVoltage(UVoltage);
        UVoltage readVoltage();
        Current readCurrent();

        // de huidige spanning van de DAC
        UVoltage currentVoltageSet;
        // bevat van elke probe het INA en DAC-adres
        static ProbeComponenten Number[3];
};