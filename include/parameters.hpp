#pragma once

// huidige versie van de transistortester
#define VERSION "v0.2 BETA"

// onderstaande DEFINE dient enkel actief te worden gezet indien debuggen op I²C-niveau vereist is.
// dit werkt niet correct wanneer het calibratiescherm wordt gebruikt!
// #define DEBUG_ACTIVE 1

// onderstaande DEFINE moet actief gezet worden indien deze op de Raspberry Pi wordt gebruikt.
// #define USING_RPI 1

// onderstaande DEFINE moet actief gezet worden indien de kalibratie-omgeving gewenst is. Dit heeft "ncurses" als dependency.
#define COMPILE_WITH_CALIBRATION_WINDOW 1

#ifndef USING_RPI
// onderstaande DEFINE moet actief gezet worden indien het op een Windows-computer wordt gecompileerd
#define WINDOWS 1

#endif

#include "base.hpp"