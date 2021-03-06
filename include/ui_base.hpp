#pragma once
// basisblokken die gebruikt worden in de opbouw van de UI (en op de achtergrond van de UI)

#include <stdio.h>
#include <string.h>
#include "probe.hpp"

#define ABS(a)	   (((a) < 0) ? -(a) : (a))
// bepaal indien beide waardes maximaal ongeveer gelijk zijn, max een procent% verschil
#define ALMOSTEQUAL(value1, value2, percentage) (ABS(ABS(value1) - ABS(value2)) < percentage * ABS(value1))

enum ComponentType {
    RESISTOR       = 0,
    CAPACITOR      = 1,
    DIODE          = 2,
    BJT_NPN        = 3,
    BJT_PNP        = 4,
    MOSFET_NMOS    = 5,
    MOSFET_PMOS    = 6,
    MOSFET_JFET    = 7,
    UNKNOWN_DEVICE = 8
};

enum ConnectionStatus {
    Connected = 0,
    NotConnected = 1,
    BadConnection = 2,
    UnusableConnection = 3
};

struct DUTInformation {
    bool isSuggestedType;
    ProbeCombination orientation;
};

struct Point {
    float x, y;
};

enum GraphType {
    IB_IC = 0,
    VCE_IC = 1,
    VBE_IC = 2
};

struct Graph {
    Point* data = nullptr;
    static GraphType graphType;
    static unsigned int nPoints;
    static int maxX, minX, maxYCurrent, minYCurrent, maxYVoltage, minYVoltage;
    static Graph graphCurrent[];
    static Graph graphVoltage[];
};

struct GraphContext {
    const char* xUnit,* yUnit1,* yUnit2;
    const char* graphTitle;
    const char* buttonDiscription[3];
    const float scaleFactorX, scaleFactorY1, scaleFactorY2;
    const bool canMeasureVoltage;
    static const GraphContext data[];
};

enum PropertyType {
    // description lijnen die extra informatie over de component weergeven
    DESCRIPTION_LINE_1 = 0,
    DESCRIPTION_LINE_2 = 1,
    DESCRIPTION_LINE_3 = 2,
    DESCRIPTION_LINE_4 = 3,
    // description lijnen die de pinout benoemen
    PINOUT_1           = 5,
    PINOUT_2           = 6,
    PINOUT_3           = 7,
    // symbool naam voor de UI
    SYMBOL_NAME        = 8,
    // component naam voor de UI
    COMPONENT_NAME     = 9,
    // component naam voor save file
    TYPE_NAME          = 10
};

struct Component {
    static ComponentType type;
    static Component* currentComponent;
    virtual void getPropertyText(PropertyType property, char* buffer) = 0;
    virtual ~Component() {}
    protected:
        ProbeCombination pinout;
        Component(ProbeCombination& p) : pinout(p) {}
};