#pragma once

// dit bestand dient als "basis" om basisfunctionaliteit overal te voorzien

#include <time.h>  // voor sleep()
#include <errno.h> // voor sleep()

// huidige versie van de transistortester
#define VERSION "v1.0"

// onderstaande DEFINE moet actief gezet worden indien de kalibratie-omgeving gewenst is. Dit heeft "ncurses" als dependency.
#define COMPILE_WITH_CALIBRATION_WINDOW 1

int sleep_ms(long ms);