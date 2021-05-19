#include "resistor.hpp"

DUTInformation Resistor::checkIfResistor() {
    DUTInformation result;
    result.isSuggestedType = false;
    for (unsigned char i = 0; i < 3; ++i) {
        // derde probe afleggen
        Probe::combinations[i].third->turnOff();
        // tweede probe als grond instellen
        Probe::combinations[i].second->setVoltage(0);
        // eerste probe als VCC instellen (1V)
        Probe::combinations[i].first->setVoltage(1000);
        // kort wachten
        sleep_ms(10);
        // stromen meten
        Current firstCurrent = Probe::combinations[i].first->readAverageCurrent(10);
        Current secondCurrent = Probe::combinations[i].second->readAverageCurrent(10);
        // kijken indien de stroom groot genoeg is (max 20K weerstanden ondersteund tegen meetfouten), met maximaal een 1% fout
        if (firstCurrent < -25 && secondCurrent > 25 && ALMOSTEQUAL(firstCurrent, secondCurrent, 0.01)) {
            result.isSuggestedType = true;
            result.orientation = Probe::combinations[i];
            // probes afleggen
            Probe::combinations[i].first->turnOff();
            Probe::combinations[i].second->turnOff();
            return result;
        }
    }
    return result;
}

void Resistor::measure() {
    // derde probe afleggen
    pinout.third->turnOff();
    // tweede probe als grond instellen
    pinout.second->setVoltage(0);
    // eerste probe instellen als bron (1V)
    pinout.first->setVoltage(1000);
    // kort wachten
    sleep_ms(10);
    // meten
    MeasureResult result1 = pinout.first->doFullMeasure(10);
    MeasureResult result2 = pinout.second->doFullMeasure(10);
    resistance = ((double) (result1.avgV - result2.avgV)) * 1000 / result2.avgA;
    pinout.first->turnOff();
    pinout.second->turnOff();
}

void Resistor::getPropertyText(PropertyType property, char* buffer) {
    switch (property) {
        case DESCRIPTION_LINE_1: {
            sprintf(buffer, "Gemeten weerstandswaarde: %f Ohm", resistance);
            break;
        }
        case DESCRIPTION_LINE_2: {
            buffer[0] = '\0';
            break;
        }
        case DESCRIPTION_LINE_3: {
            buffer[0] = '\0';
            break;
        }
        case DESCRIPTION_LINE_4: {
            buffer[0] = '\0';
            break;
        }
        case PINOUT_1: {
            sprintf(buffer, "Pin %d", pinout.firstPinNumber);
            break;
        }
        case PINOUT_2: {
            buffer[0] = '\0';
            break;
        }
        case PINOUT_3: {
            sprintf(buffer, "Pin %d", pinout.secondPinNumber);
            break;
        }
        case SYMBOL_NAME: {
            strcpy(buffer, "../ui/resistor.png");
            break;
        }
        case COMPONENT_NAME: {
            strcpy(buffer, "Weerstand");
            break;
        }
        case TYPE_NAME: {
            strcpy(buffer, "RESISTOR");
            break;
        }
        default:
            buffer[0] = '\0';
    }
}