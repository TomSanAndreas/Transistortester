#pragma once

#include "ui_base.hpp"

struct Resistor : Component {
    double resistance;
    // gebruikt om te kijken of de DUT een weerstand is
    static DUTInformation checkIfResistor();
    // eigenschappen in tekst uitdrukken
    void getPropertyText(PropertyType property, char* buffer);
    Resistor(ProbeCombination& p) : Component(p) {
        measure();
    }
    private:
        void measure();
};