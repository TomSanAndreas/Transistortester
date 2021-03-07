#include "probe.hpp"

Probe Probe::first = Probe(DAC_Address::A0, INA_Address::INA1);
Probe Probe::second = Probe(DAC_Address::A1, INA_Address::INA2);
Probe Probe::third = Probe(DAC_Address::A2, INA_Address::INA3);

Probe::Probe(DAC_Address dac_addr, INA_Address ina_addr) 
: dac(dac_addr)
, ina(ina_addr) {}

void Probe::setVoltage(UVoltage newVoltage) {
    dac.setVoltage(newVoltage);
}

UVoltage Probe::readVoltage() {
    return ina.readVoltage();
}

Current Probe::readCurrent() {
    return ina.readCurrent();
}