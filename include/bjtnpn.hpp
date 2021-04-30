#pragma once

#include "ui_base.hpp"

struct BjtNpn : Component {
    double averageBeta;
    double minBeta;
    double maxBeta;
    double minVbeVoltage;
    // gebruikt om te kijken of de DUT een NPN transistor is
    static DUTInformation checkIfNPN();
    // genereren grafieken
    void generateIbIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint, bool sampleVoltage);
    void generateVceIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint);
    // eigenschappen in tekst uitdrukken
    void getPropertyText(PropertyType property, char* buffer);
    // pinout: first - collector, second - base, third - emitter
    BjtNpn(ProbeCombination& p) : Component(p) {
        measure();
    }
    private:
        // zoekt van de transistor de kleinst mogelijke VBE 
        void setLowestVBE();
        // bepaalt basiseigenschappen van de transistor
        void measure();
};