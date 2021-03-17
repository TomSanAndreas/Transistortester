#include "parameters.hpp"
#include <string.h>
#include <stdio.h>

#ifdef COMPILE_WITH_CALIBRATION_WINDOW
#include "calibration_window.hpp"
#else
#include "probe.hpp"
#endif

// compileren via "make"


int main(int argc, char** argv) {
    if (argc == 2) {
        if (strcmp(argv[1], "-k") == 0 || strcmp(argv[1], "--kalibratie") == 0) {
            #ifdef COMPILE_WITH_CALIBRATION_WINDOW
            Probe::init();
            CalibrationWindow::init();
            while (!CalibrationWindow::shouldExit()) {
                CalibrationWindow::update();
                CalibrationWindow::sleep(10);
            }
            CalibrationWindow::destroy();
            Probe::destroy();
            #else
            printf("Dit programma werd niet met het kalibratiescherm gecompileerd. Gelieve de source-code opnieuw te compileren met de COMPILE_WITH_CALIBRATION_WINDOW-vlag actief in parameters.hpp.\n");
            #endif
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Volgende argumenten zijn beschikbaar:\n");
            printf("\t-h, --help\t\tPrint deze help.\n");
            #ifdef COMPILE_WITH_CALIBRATION_WINDOW
            printf("\t-k, --kalibratie\t\tStart de kalibreeromgeving.\n");
            #else
            printf("\t-k, --kalibratie\t\tDeze functionaliteit is niet beschikbaar, zie -k voor meer informatie.\n");
            #endif
            printf("\t-s, --start\t\tStart de grafische toepassing (standaard).\n");
        } else if (strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--start")) {
            goto gewone_start;
        } else {
            printf("Argument %s is niet herkend. Zie <-h, --help> voor meer informatie.\n", argv[1]);
        }
    } else if (argc > 2) {
        printf("Gelieve maximaal 1 extra argument mee te geven. Zie argument <-h, --help> voor meer uitleg.\n");
    } else {
        gewone_start:
        // TODO: start grafische applicatie voor gewoon gebruik
        printf("Deze functionaliteit zit nog niet ingebouwd.\n");
    }

    return 0;
}