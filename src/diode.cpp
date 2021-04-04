#include "diode.hpp"

DiodeType DiodeType::possibleTypes[] = {
    { "Schottky diode", { 200, 205, 210 } }, { "Germanium diode", { 250, 300, 310 } }, { "Silicium diode", { 600, 700, 710 } }
};

DUTInformation Diode::checkIfDiode() {
    DUTInformation result;
    result.isSuggestedType = false;
    for (unsigned char i = 0; i < 6; ++i) {
        // turn off the third probe
        Probe::combinations[i].third->turnOff();
        // set the second probe as GND
        Probe::combinations[i].second->setVoltage(0);
        // set the first probe as 710mV, if the component is a diode with a lower voltage drop,
        // there will be a bigger current, but 0.7V is not enough to break a diode when reverse
        // biased so this should be safe
        Probe::combinations[i].first->setVoltage(710);
        // wait for a short time
        sleep_ms(10);
        // check if a current flows, with a 5% margin of error
        Current anodeCurrent = Probe::combinations[i].first->readAverageCurrent(10);
        Current cathodeCurrent = Probe::combinations[i].second->readAverageCurrent(10);
        if (ALMOSTEQUAL(anodeCurrent, cathodeCurrent, 0.05) && anodeCurrent < - 15 && cathodeCurrent > 15) {
            // check if no current flows in reverse bias
            Probe::combinations[i].second->setVoltage(710);
            Probe::combinations[i].first->setVoltage(0);
            sleep_ms(10);
            anodeCurrent = Probe::combinations[i].first->readAverageCurrent(10);
            cathodeCurrent = Probe::combinations[i].second->readAverageCurrent(10);
            if (ABS(anodeCurrent) < 2 && ABS(cathodeCurrent) < 2) {
                result.isSuggestedType = true;
                result.orientation = Probe::combinations[i];
                // turn probes off
                Probe::combinations[i].first->turnOff();
                Probe::combinations[i].second->turnOff();
                return result;
            }
        }
    }
    return result;
}

void Diode::measure() {
    // turn off the third probe
    pinout.third->turnOff();
    // set the second probe as GND
    pinout.second->setVoltage(0);
    // set the first probe as different voltage sources (Schottky - 200mV, Germanium - 250-300mV, Silicon - 600-700mV)
    UVoltage VCC;
    for (unsigned char j = 0; j < 3; ++j) {
        for (unsigned char k = 0; k < 3; ++k) {
            VCC = DiodeType::possibleTypes[j].voltages[k];
            pinout.first->setVoltage(VCC);
            // wait for a short time
            sleep_ms(10);
            // check if current flows
            Current anodeCurrent = pinout.first->readAverageCurrent(10);
            if (anodeCurrent < -10) {
                // set properties
                voltageDrop = (((double) pinout.first->readAverageVoltage(10)) - pinout.second->readAverageVoltage(10)) / 1000;
                type = DiodeType::possibleTypes[j];
                // turn probes off
                pinout.first->turnOff();
                pinout.second->turnOff();
                return;
            }
        }
    }
}

void Diode::getPropertyText(PropertyType property, char* buffer) {
    switch (property) {
        case DESCRIPTION_LINE_1: {
            sprintf(buffer, "Spanningsval: %f", voltageDrop);
            break;
        }
        case DESCRIPTION_LINE_2: {
            sprintf(buffer, "Type: %s", type.typeName);
            break;
        }
        case DESCRIPTION_LINE_3: {
            sprintf(buffer, "Bereik spanningsval: %dmV - %dmV", type.voltages[0], type.voltages[3]);
            break;
        }
        case DESCRIPTION_LINE_4: {
            buffer[0] = '\0';
            break;
        }
        case PINOUT_1: {
            sprintf(buffer, "Pin %d: Anode", pinout.firstPinNumber);
            break;
        }
        case PINOUT_2: {
            buffer[0] = '\0';
            break;
        }
        case PINOUT_3: {
            sprintf(buffer, "Pin %d: Kathode", pinout.secondPinNumber);
            break;
        }
        case SYMBOL_NAME: {
            strcpy(buffer, "../ui/diode.png");
            break;
        }
        case COMPONENT_NAME: {
            strcpy(buffer, "Diode");
            break;
        }
        default:
            buffer[0] = '\0';
    }
}