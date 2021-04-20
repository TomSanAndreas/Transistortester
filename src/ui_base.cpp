#include "ui_base.hpp"

const unsigned int GraphContext::IB_IC = 0;
const unsigned int GraphContext::IC_VCE = 1;
const unsigned int GraphContext::IC_VBE = 2;

const GraphContext GraphContext::data[] = {
    {"uA", "mA", "mV", "IC en VCE i.f.v. IB",  {"IC en VCE i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}},
    {"mA", "mV", "", "VCE i.f.v. IC", {"IC en VCE i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}},
    {"mA", "mV", "", "VBE i.f.v. IC", {"IC en VCE i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}}
};

unsigned int Graph::graphType;

int Graph::minX;
int Graph::maxX;

int Graph::maxYCurrent;
int Graph::minYCurrent;

int Graph::maxYVoltage;
int Graph::minYVoltage;

Graph Graph::graphCurrent[3];
Graph Graph::graphVoltage[3];

ComponentType Component::type = UNKNOWN_DEVICE;
Component* Component::currentComponent = nullptr;