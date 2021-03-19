#pragma once

#include "parameters.hpp"
#ifdef COMPILE_WITH_CALIBRATION_WINDOW

#ifdef USE_RPI
#include <ncurses.h>
#else
#ifdef WINDOWS
#include <ncurses/ncurses.h>
#endif
#endif

#include "probe.hpp"

namespace CalibrationWindow {
    extern bool shouldExit;

    void init();
    void update();
    void destroy();
}
#endif