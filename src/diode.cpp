#include "diode.hpp"

DiodeType DiodeType::possibleTypes[] = {
    { "Schottky diode", { 200, 205, 210 } }, { "Germanium diode", { 250, 300, 310 } }, { "Silicium diode", { 600, 700, 710 } }
};

DUTInformation Diode::checkIfDiode() {
    DUTInformation result;
    result.isSuggestedType = false;
    for (unsigned char i = 0; i < 6; ++i) {
        // leg de derde probe af
        Probe::combinations[i].third->turnOff();
        // de tweede probe instellen als grond
        Probe::combinations[i].second->setVoltage(0);
        // stel de eerste probe in op 710mV, indien de component een diode is met een lagere spanningsval, dan
        // is er een grote stroom, maar 0.7V is zeker niet te groot om een diode mee omgekeerd te biasen,
        // waardoor dit een veilige spanning is om mee te testen
        Probe::combinations[i].first->setVoltage(710);
        // kort wachten
        sleep_ms(10);
        // kijk of er aan beide terminals een ongeveer gelijkaardige stroom vloeit (los van het teken) en kijk naar het teken zelf
        Current anodeCurrent = Probe::combinations[i].first->readAverageCurrent(10);
        Current cathodeCurrent = Probe::combinations[i].second->readAverageCurrent(10);
        if (ALMOSTEQUAL(anodeCurrent, cathodeCurrent, 0.05) && anodeCurrent < - 15 && cathodeCurrent > 15) {
            // controleer dat er geen stroom vloeit in de tegengestelde zin
            Probe::combinations[i].second->setVoltage(710);
            Probe::combinations[i].first->setVoltage(0);
            sleep_ms(10);
            anodeCurrent = Probe::combinations[i].first->readAverageCurrent(10);
            cathodeCurrent = Probe::combinations[i].second->readAverageCurrent(10);
            if (ABS(anodeCurrent) < 2 && ABS(cathodeCurrent) < 2) {
                result.isSuggestedType = true;
                result.orientation = Probe::combinations[i];
                // probes afleggen
                Probe::combinations[i].first->turnOff();
                Probe::combinations[i].second->turnOff();
                return result;
            }
        }
    }
    return result;
}

void Diode::measure() {
    // leg de derde probe af
    pinout.third->turnOff();
    // stel de tweede probe in als grond
    pinout.second->setVoltage(0);
    // zet de eerste probe als een bepaalde spanningsbron (afhankelijk van het geteste type)
    UVoltage VCC;
    for (unsigned char j = 0; j < 3; ++j) {
        for (unsigned char k = 0; k < 3; ++k) {
            VCC = DiodeType::possibleTypes[j].voltages[k];
            pinout.first->setVoltage(VCC);
            // kort wachten
            sleep_ms(10);
            // kijken indien stroom vloeit
            Current anodeCurrent = pinout.first->readAverageCurrent(10);
            if (anodeCurrent < -1000) {
                // eigenschappen instellen
                forwardVoltage = (((double) pinout.first->readAverageVoltage(10)) - pinout.second->readAverageVoltage(10)) / 1000;
                forwardCurrent = pinout.second->readAverageCurrent(10) / 1000.0;
                type = DiodeType::possibleTypes[j];
                // probes afleggen
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
            sprintf(buffer, "Voorwaarste spanningsval is %f V,", forwardVoltage);
            break;
        }
        case DESCRIPTION_LINE_2: {
            sprintf(buffer, "bij een stroom van %f mA.", forwardCurrent);
            break;
        }
        case DESCRIPTION_LINE_3: {
            sprintf(buffer, "Type: %s", type.typeName);
            break;
        }
        case DESCRIPTION_LINE_4: {
            sprintf(buffer, "Bereik spanningsval: %dmV - %dmV", type.voltages[0], type.voltages[2]);
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
        case TYPE_NAME: {
            strcpy(buffer, "DIODE");
            break;
        }
        default:
            buffer[0] = '\0';
    }
}