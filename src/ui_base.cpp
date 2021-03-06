#include "ui_base.hpp"

const GraphContext GraphContext::data[] = {
    {"uA", "mA", "mV", "IC en VCE i.f.v. IB",  {"IC en VCE i.f.v. IB", "IC i.f.v. VCE", "IC i.f.v. VBE"}, 1.0, 1000.0, 1.0, true},
    {"mV", "uA", "", "IC i.f.v. VCE", {"IC en VCE i.f.v. IB", "IC i.f.v. VCE", "IC i.f.v. VBE"}, 1.0, 1.0, 1.0, false},
    {"mV", "mA", "", "IC i.f.v. VBE", {"IC en VCE i.f.v. IB", "IC i.f.v. VCE", "IC i.f.v. VBE"}, 1.0, 1000.0, 1.0, false}
};

GraphType Graph::graphType;

unsigned int Graph::nPoints = 0;

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