#include "parameters.hpp"
#include <string.h>
#include <stdio.h>

#include "user_interface.hpp"

#ifdef COMPILE_WITH_CALIBRATION_WINDOW
#include "calibration_window.hpp"
#endif

int main(int argc, char** argv) {
    if (argc == 2) {
        if (strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--shell") == 0) {
            #ifdef COMPILE_WITH_CALIBRATION_WINDOW
            Probe::init();
            CalibrationWindow::init();
            while (!CalibrationWindow::shouldExit) {
                CalibrationWindow::update();
                sleep_ms(10);
            }
            CalibrationWindow::destroy();
            Probe::destroy();
            #else
            printf("Dit programma werd niet met het kalibratiescherm gecompileerd. Gelieve de source-code opnieuw te compileren met de COMPILE_WITH_CALIBRATION_WINDOW-vlag actief in base.hpp.\n");
            #endif
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Volgende argumenten zijn beschikbaar:\n"
                   "\t-h, --help          Print deze help.\n");
            #ifdef COMPILE_WITH_CALIBRATION_WINDOW
            printf("\t-s, --shell         Start de interactieve terminal-interface.\n");
            #else
            printf("\t-s, --shell         Deze functionaliteit is niet beschikbaar, zie -s voor meer informatie.\n");
            #endif
            printf("\t<geen argumenten>   Start de grafische interface voor gewoon gebruik.\n");
        } else {
            printf("Argument %s is niet herkend. Zie <-h, --help> voor meer informatie.\n", argv[1]);
        }
    } else if (argc > 2) {
        printf("Gelieve maximaal 1 extra argument mee te geven. Zie argument <-h, --help> voor meer uitleg.\n");
    } else {
        UserInterface::init(&argc, &argv);
    }
    return 0;
}