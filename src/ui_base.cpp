#include "ui_base.hpp"

const unsigned int GraphContext::IB_IC = 0;
const unsigned int GraphContext::IC_VCE = 1;
const unsigned int GraphContext::IC_VBE = 2;

const GraphContext GraphContext::data[] = {
    {"uA", "mA", "IC i.f.v. IB",  {"IC i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}},
    {"mA", "mV", "VCE i.f.v. IC", {"IC i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}},
    {"mA", "mV", "VBE i.f.v. IC", {"IC i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}}
};

unsigned int Graph::graphType;

int Graph::maxX;
int Graph::maxY;
int Graph::minX;
int Graph::minY;

Graph Graph::graphs[3];

ComponentType Component::type = UNKNOWN_DEVICE;
Component* Component::currentComponent = nullptr;