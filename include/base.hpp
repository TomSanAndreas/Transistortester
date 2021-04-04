#pragma once

// this file contains basic functions

#include <time.h>  // voor sleep()
#include <errno.h> // voor sleep()

// huidige versie van de transistortester
#define VERSION "v0.6 PRE-RELEASE"

// onderstaande DEFINE moet actief gezet worden indien de kalibratie-omgeving gewenst is. Dit heeft "ncurses" als dependency.
#define COMPILE_WITH_CALIBRATION_WINDOW 1

int sleep_ms(long ms);