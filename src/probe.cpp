#include "probe.hpp"

#include <stdio.h>
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
    // fill up LUT as voltage reference during measurements
    for (unsigned int i = 0; i < 4096; ++i) {
        dac.setVoltage(i);
        exactVoltageLUT[i] = ina.readVoltage() + vOffset;
    }
    // determine voltBitRatio, max voltage is already set in for-loop
    // get max voltage
    UVoltage maxVoltage = exactVoltageLUT[4095];
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
    currentVoltageBitsSet = newVoltage / voltBitRatio + .5;
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

MeasureResult Probe::doFullMeasure(unsigned int nSamples) {
    MeasureResult result;
    result.usedVoltage = exactVoltageLUT[currentVoltageBitsSet];
    long long sumV = 0, sumA = 0;
    Current measuredA;
    UVoltage measuredV;
    for (unsigned int i = 0; i < nSamples; ++i) {
        measuredA = ina.readCurrent();
        measuredV = ina.readVoltage();
        if (measuredA > result.maxA) {
            result.maxA = measuredA;
        }
        if (measuredA < result.minA) {
            result.minA = measuredA;
        }
        if (measuredV > result.maxV) {
            result.maxV = measuredV;
        }
        if (measuredV < result.minV) {
            result.minV = measuredV;
        }
        sumA += measuredA;
        sumV += measuredV;
        sleep(1);
    }
    result.avgA = sumA / nSamples;
    result.avgV = sumV / nSamples;
    return result;
}