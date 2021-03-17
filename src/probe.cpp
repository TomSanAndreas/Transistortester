#include "probe.hpp"

Probe* Probe::probe = nullptr;

void Probe::init() {
    probe = new Probe[3] { {DAC_Address::A0, INA_Address::INA1}, {DAC_Address::A2, INA_Address::INA2}, {DAC_Address::A1, INA_Address::INA3} };
}

void Probe::destroy() {
    delete[] probe;
}

Probe::Probe(DAC_Address dac, INA_Address ina) 
: dac(dac)
, ina(ina) {}

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