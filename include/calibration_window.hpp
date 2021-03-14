#pragma once

#include "parameters.hpp"

#ifdef USE_RPI
#include <ncurses.h>
#else
#ifdef WINDOWS
#include <ncurses/ncurses.h>
#endif
#endif

#include "probe.hpp"

namespace CalibrationWindow {
    void init();
    void update();
    int sleep(long ms);
    bool shouldExit();
    void destroy();

    extern Probe *eersteProbe, *tweedeProbe, *derdeProbe;
}