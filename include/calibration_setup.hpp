#pragma once

#include <iostream>
#include "base.hpp"
#include "parameters.hpp"
#include "probe.hpp"

namespace CalibrationSetup {
    extern bool doneCalibrating;
    
    void initClean();
    void initFromFile();
    void update();
    void cleanUp();
}