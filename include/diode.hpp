#pragma once

#include "ui_base.hpp"

struct DiodeType {
    const char* typeName;
    // min voltage, average voltage, max voltage for the type
    UVoltage voltages[3];
    static DiodeType possibleTypes[];
};

struct Diode : Component {
    double forwardVoltage;
    Current forwardCurrent;
    DiodeType& type;
    // gebruikt om te kijken of de DUT een diode is
    static DUTInformation checkIfDiode();
    // eigenschappen in tekst uitdrukken
    void getPropertyText(PropertyType property, char* buffer);
    Diode(ProbeCombination& p) : Component(p), type(DiodeType::possibleTypes[0]) {
        measure();
    }
    private:
        // bepaalt basiseigenschappen van de diode
        void measure();
};
