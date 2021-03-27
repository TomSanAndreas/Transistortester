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
    voltBitRatio = ((double) maxVoltage) / 4095;
    // set the dac back
    dac.setVoltage(original);
}

void Probe::setOffset(Voltage offset) {
    vOffset = offset;
}

void Probe::setVoltage(UVoltage newVoltage) {
    currentVoltageSet = newVoltage;
    dac.setVoltage(newVoltage / voltBitRatio + .5); // the addition of 0.5 makes it so x.5 and up would round up when casted to an int
}

void Probe::setShunt(float newShunt) {
    ina.rShunt = newShunt;
}

float Probe::adjustShuntUsingCurrent(Current expectedCurrent) {
    Voltage shuntVoltage = ina.readShuntVoltage();
    ina.rShunt = shuntVoltage / expectedCurrent;
    return ina.rShunt;
}

Voltage Probe::readShuntVoltage() {
    return ina.readShuntVoltage();
}

Voltage Probe::readAverageShuntVoltage(unsigned int nSamples) {
    long long sum = 0;
    for (unsigned int i = 0; i < nSamples; ++i) {
        sum += ina.readShuntVoltage();
        sleep(1);
    }
    return sum / nSamples;
}

UVoltage Probe::readVoltage() {
    return ina.readVoltage() + vOffset;
}

UVoltage Probe::readAverageVoltage(unsigned int nSamples) {
    unsigned long long sum = 0;
    for (unsigned int i = 0; i < nSamples; ++i) {
        sum += ina.readVoltage();
        sleep(1);
    }
    return sum / nSamples + vOffset;
}

Current Probe::readCurrent() {
    return ina.readCurrent();
}

Current Probe::readAverageCurrent(unsigned int nSamples) {
    long long sum = 0;
    for (unsigned int i = 0; i < nSamples; ++i) {
        sum += ina.readCurrent();
        sleep(1);
    }
    return sum / nSamples;
}