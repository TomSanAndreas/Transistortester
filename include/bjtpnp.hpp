#pragma once

#include "ui_base.hpp"

struct BjtPnp : Component {
    double averageBeta;
    double minBeta;
    double maxBeta;
    double minVbeVoltage;
    // gebruikt om te kijken of de DUT een PNP transistor is
    static DUTInformation checkIfPNP();

    // grafiek genereren met de gegeven eigenschappen
    void generateIbIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint);
    // eigenschappen in tekst uitdrukken
    void getPropertyText(PropertyType property, char* buffer);
    // pinout: first - collector, second - base, third - emitter
    BjtPnp(ProbeCombination& p) : Component(p) {
        measure();
    }
    private:
        // zoekt van de transistor de kleinst mogelijke VBE 
        void setLowestVBE();
        // bepaalt basiseigenschappen van de transistor
        void measure();
};