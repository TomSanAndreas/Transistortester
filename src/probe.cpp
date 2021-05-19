#include "probe.hpp"
#include <stdio.h>
Probe* Probe::probe = nullptr;
ProbeCombination* Probe::combinations;

void Probe::init() {
    probe = new Probe[3] { {DAC_Address::A0, INA_Address::INA1}, {DAC_Address::A2, INA_Address::INA2}, {DAC_Address::A1, INA_Address::INA3} };
    combinations = new ProbeCombination[6] {
        { &probe[0], &probe[1], &probe[2], 1, 2, 3 },
        { &probe[1], &probe[2], &probe[0], 2, 3, 1 },
        { &probe[0], &probe[2], &probe[1], 1, 3, 2 },
        { &probe[1], &probe[0], &probe[2], 2, 1, 3 },
        { &probe[2], &probe[1], &probe[0], 3, 2, 1 },
        { &probe[2], &probe[0], &probe[1], 3, 1, 2 }
    };
}

void Probe::destroy() {
    delete[] combinations;
    delete[] probe;
}

Probe::Probe(DAC_Address dac, INA_Address ina) 
: dac(dac)
, ina(ina) {}

void Probe::calibrate(void (*updateProgress)(), double* progress) {
    // huidige spanning bewaren
    UVoltage original = dac.currentVoltage;
    // LUT opvullen met spanningsreferenties tijdens het meten
    for (unsigned int i = 0; i < 4096; ++i) {
        dac.setVoltage(i);
        exactVoltageLUT[i] = readAverageVoltage(3);
        // vooruitgang weergeven
        *progress = i / 4096.0;
        updateProgress();
    }
    // voltBitRatio bepalen, maxspanning is al ingesteld via de for-lus
    UVoltage maxVoltage = exactVoltageLUT[4095];
    voltBitRatio = ((double) maxVoltage) / 4095;
    // oorspronkelijke spanning terugzetten
    dac.setVoltage(original);
}

void Probe::turnOff() {
    isDacTurnedOn = false;
    dac.turnOff();
}

void Probe::setOffset(Voltage offset) {
    vOffset = offset;
}

void Probe::setVoltage(UVoltage newVoltage) {
    if (!isDacTurnedOn) {
        dac.turnOn();
        isDacTurnedOn = true;
    }
    currentVoltageSet = newVoltage;
    currentVoltageBitsSet = newVoltage / voltBitRatio + .5; // de extra .5 helpt bij het afronden juist voor het casten naar int
    dac.setVoltage(currentVoltageBitsSet);
}

void Probe::increaseVoltage() {
    ++currentVoltageBitsSet;
    currentVoltageSet = exactVoltageLUT[currentVoltageBitsSet];
    dac.setVoltage(currentVoltageBitsSet);
}

void Probe::decreaseVoltage() {
    --currentVoltageBitsSet;
    currentVoltageSet = exactVoltageLUT[currentVoltageSet];
    dac.setVoltage(currentVoltageBitsSet);
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
    }
    return sum / nSamples + vOffset;
}

Current Probe::readCurrent() {
    return ina.readCurrent();
}

Current Probe::readAverageCurrent(unsigned int nSamples) {
    double sum = 0;
    for (unsigned int i = 0; i < nSamples; ++i) {
        sum += ina.readCurrent();
    }
    return sum / nSamples;
}

MeasureResult Probe::doFullMeasure(unsigned int nSamples) {
    MeasureResult result;
    result.usedVoltage = exactVoltageLUT[currentVoltageBitsSet];
    long long sumV = 0;
    double sumA = 0;
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
    }
    result.avgA = sumA / nSamples;
    result.avgV = sumV / nSamples;
    return result;
}
