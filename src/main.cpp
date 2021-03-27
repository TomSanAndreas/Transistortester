#include "parameters.hpp"
#include <string.h>
#include <stdio.h>

#include "calibration_setup.hpp"

#ifdef COMPILE_WITH_CALIBRATION_WINDOW
#include "calibration_window.hpp"
#endif

// compileren via "make"


int main(int argc, char** argv) {
    if (argc == 2) {
        if (strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--shell") == 0) {
            #ifdef COMPILE_WITH_CALIBRATION_WINDOW
            Probe::init();
            CalibrationWindow::init();
            while (!CalibrationWindow::shouldExit) {
                CalibrationWindow::update();
                sleep(10);
            }
            CalibrationWindow::destroy();
            Probe::destroy();
            #else
            printf("Dit programma werd niet met het kalibratiescherm gecompileerd. Gelieve de source-code opnieuw te compileren met de COMPILE_WITH_CALIBRATION_WINDOW-vlag actief in parameters.hpp.\n");
            #endif
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Volgende argumenten zijn beschikbaar:\n"
                   "\t-h, --help          Print deze help.\n");
            #ifdef COMPILE_WITH_CALIBRATION_WINDOW
            printf("\t-k, --kalibratie    Start het kalibreerproces.\n");
            #else
            printf("\t-s, --shell         Deze functionaliteit is niet beschikbaar, zie -s voor meer informatie.\n");
            #endif
            printf("\t-s, --shell         Start de interactieve terminal-interface.\n"
                   "\t<geen argumenten>   Start de grafische interface voor gewoon gebruik.\n");
        } else if (strcmp(argv[1], "-k") == 0 || strcmp(argv[1], "--kalibratie") == 0) {
            CalibrationSetup::initClean();
            while (!CalibrationSetup::doneCalibrating) {
                CalibrationSetup::update();
            }
            CalibrationSetup::cleanUp();
        } else {
            printf("Argument %s is niet herkend. Zie <-h, --help> voor meer informatie.\n", argv[1]);
        }
    } else if (argc > 2) {
        printf("Gelieve maximaal 1 extra argument mee te geven. Zie argument <-h, --help> voor meer uitleg.\n");
    } else {
        // TODO: start grafische applicatie voor gewoon gebruik
        printf("Deze functionaliteit zit nog niet ingebouwd.\n");
    }

    return 0;
}