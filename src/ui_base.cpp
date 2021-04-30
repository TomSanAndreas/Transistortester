#include "ui_base.hpp"

const GraphContext GraphContext::data[] = {
    {"uA", "mA", "mV", "IC en VCE i.f.v. IB",  {"IC en VCE i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}, 1.0, 1000.0, 1.0},
    {"mA", "mV", "", "VCE i.f.v. IC", {"IC en VCE i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}, 1000.0, 1.0, 1.0},
    {"mA", "mV", "", "VBE i.f.v. IC", {"IC en VCE i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}, 1000.0, 1.0, 1.0}
};

GraphType Graph::graphType;

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