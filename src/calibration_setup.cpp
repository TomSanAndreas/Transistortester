#include "calibration_setup.hpp"
#include <map>

bool CalibrationSetup::doneCalibrating = false;

struct Combinatie {
    uint8_t first, second;
    static const Combinatie combinatie[];
    private:
        Combinatie(uint8_t a , uint8_t b) : first(a), second(b) {}
};

const Combinatie Combinatie::combinatie[] = { {0, 1}, {1, 2}, {0, 2} };

void CalibrationSetup::initClean() {
    Probe::init();
    printf("Kalibratie is gestart, gelieve niets aan te sluiten en op enter te drukken.");
    std::cin.get();
    Voltage offset;
    for (uint8_t i = 0; i < 3; ++i) {
        offset = - Probe::probe[i].readAverageVoltage(10) + 1; // toch eentje minder nemen zodat de spanning (bijna) nooit onder 0 kan worden gemeten 
        printf("Offset probe %d wordt ingesteld op %d mV.\n", i + 1, offset);
        Probe::probe[i].setOffset(offset);
        Probe::probe[i].calibrate();
    }
}

void CalibrationSetup::initFromFile() {
    // TODO: inlezen bestand die opgeslagen data bevat, deze in map steken
}

float getResistance(short lowerBound, char lowerBoundUnit, short upperBound, char upperBoundUnit) {
    char buffer[50];
    printf("Gelieve een weerstand tussen %d%c en %d%c "
    "te nemen.\nGekozen weerstandswaarde "
    "(zo precies mogelijk en eindigend op [R] of k): ",
    lowerBound, lowerBoundUnit, upperBound,
    upperBoundUnit);
    gets(buffer);
    unsigned int i = 0;
    float result = 0;
    unsigned int decimalCorrection = 1;
    bool hasPassedDecimalPoint = false;
    while (buffer[i] == ' ') { ++i; }
    while (buffer[i] != '\0') {
        if (buffer[i] >= '0' && buffer[i] <= '9') {
            result *= 10;
            result += buffer[i] - '0';
            if (hasPassedDecimalPoint) {
                decimalCorrection *= 10;
            }
        } else if ((buffer[i] == 'r' || buffer[i] == 'R') && buffer[i + 1] == '\0') {
            return result / decimalCorrection;
        } else if ((buffer[i] == 'k' || buffer[i] == 'K') && buffer[i + 1] == '\0') {
            return result * 1000 / decimalCorrection;
        } else if ((buffer[i] == '.' || buffer[i] == ',') && !hasPassedDecimalPoint) {
            hasPassedDecimalPoint = true;
        } else {
            return -1;
        }
        ++i;
    }
    return result / decimalCorrection;
}

void CalibrationSetup::update() {
    float r = getResistance(50, 'R', 500, 'K');
    while (r < 0) {
        printf("Ongeldige weerstand. "),
        r = getResistance(50, 'R', 500, 'K');
    }
    for (uint8_t i = 0; i < 3; ++i) {
        printf("Gelieve de gekozen weerstand aan te sluiten tussen probe %d en %d "
               "en daarna op enter te drukken.", Combinatie::combinatie[i].first + 1, Combinatie::combinatie[i].second + 1);
        std::cin.get();
        printf("Kalibreren.\n");
        for (UVoltage v = 100; v <= 5000; v += 100) {
            Probe::probe[Combinatie::combinatie[i].first].setVoltage(v);
            sleep(5);
            UVoltage voltage = Probe::probe[Combinatie::combinatie[i].first].readAverageVoltage(5) - Probe::probe[Combinatie::combinatie[i].second].readAverageVoltage(5);
            Current expectedCurrent = voltage / (r / 1000);
            printf("%dmV - %duA verwacht.\n", v, expectedCurrent);
        }
    }
    // printf("Afwijkingen worden bepaald.\n");
    // for (UVoltage i = 10; i <= 5000; ++i) {
    //     Probe::probe[0].setVoltage(i);
    //     sleep(10);
    //     UVoltage voltage = Probe::probe[0].readAverageVoltage(10) - Probe::probe[1].readAverageVoltage(10);
    //     Current expectedCurrent = voltage / r;
    //     Voltage vShunt = Probe::probe[0].getShuntVoltage();
    // }
}

void CalibrationSetup::cleanUp() {
    Probe::destroy();
}