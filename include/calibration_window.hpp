#pragma once

#include "parameters.hpp"
#ifdef COMPILE_WITH_CALIBRATION_WINDOW

#ifdef WINDOWS
#include <ncurses/ncurses.h>
#else
#include <ncurses.h>
#endif

#include "probe.hpp"

namespace CalibrationWindow {
    extern bool shouldExit;

    void init();
    void update();
    void destroy();
}
#endif