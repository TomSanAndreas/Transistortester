#pragma once

#include "ui_base.hpp"

struct Capacitor : Component {
    double capacitance;
    static DUTInformation checkIfCapacitor();
    // eigenschappen in tekst uitdrukken
    void getPropertyText(PropertyType property, char* buffer);
    Capacitor(ProbeCombination& p) : Component(p) {
        measure();
    }
    private:
        void measure();
};