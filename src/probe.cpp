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
, ina(ina) {
    calibrate();
}

void Probe::calibrate() {
    // keep track of the original voltage
    UVoltage original = dac.currentVoltage;
    // set max voltage
    dac.setVoltage(4095);
    // sleep for 10 ms so the voltage stabilises
    sleep(10);
    // sample the voltage
    UVoltage maxVoltage = ina.readVoltage() + vOffset;
    // set ratio
    voltBitRatio = ((double) maxVoltage) / 4096;
    // set the dac back
    dac.setVoltage(original);
}

void Probe::setOffset(Voltage offset) {
    vOffset = offset;
}

void Probe::setVoltage(UVoltage newVoltage) {
    currentVoltageSet = newVoltage;
    dac.setVoltage(newVoltage / voltBitRatio);
}

void Probe::setShunt(float newShunt) {
    ina.rShunt = newShunt;
}

float Probe::adjustShuntUsingCurrent(Current expectedCurrent) {
    Voltage shuntVoltage = ina.readShuntVoltage();
    ina.rShunt = shuntVoltage / expectedCurrent;
    return ina.rShunt;
}

UVoltage Probe::readVoltage() {
    return ina.readVoltage() + vOffset;
}

Current Probe::readCurrent() {
    return ina.readCurrent();
}