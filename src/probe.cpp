#include "probe.hpp"

ProbeComponenten Probe::Number[3] = { {DAC_Address::A0, INA_Address::INA1}, {DAC_Address::A2, INA_Address::INA2}, {DAC_Address::A1, INA_Address::INA3} };

Probe::Probe(const ProbeComponenten& pc) 
: dac(pc.dac)
, ina(pc.ina) {}

void Probe::setVoltage(UVoltage newVoltage) {
    currentVoltageSet = newVoltage;
    dac.setVoltage(newVoltage);
}

UVoltage Probe::readVoltage() {
    return ina.readVoltage();
}

Current Probe::readCurrent() {
    return ina.readCurrent();
}