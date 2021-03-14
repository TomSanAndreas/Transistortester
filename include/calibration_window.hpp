#pragma once

#include <ncurses.h>

#include "probe.hpp"

namespace CalibrationWindow {
    void init();
    void update();
    int sleep(long ms);
    bool shouldExit();
    void destroy();

    extern Probe *eersteProbe, *tweedeProbe, *derdeProbe;
}