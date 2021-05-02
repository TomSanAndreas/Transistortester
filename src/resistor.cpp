#include "resistor.hpp"

DUTInformation Resistor::checkIfResistor() {
    DUTInformation result;
    result.isSuggestedType = false;
    for (unsigned char i = 0; i < 3; ++i) {
        // turn off the third probe
        Probe::combinations[i].third->turnOff();
        // set the second probe as GND
        Probe::combinations[i].second->setVoltage(0);
        // set the first probe as VCC (1000 mV)
        Probe::combinations[i].first->setVoltage(1000);
        // wait for a short time
        sleep_ms(10);
        // check if a current can be measured in first & second probe
        Current firstCurrent = Probe::combinations[i].first->readAverageCurrent(10);
        Current secondCurrent = Probe::combinations[i].second->readAverageCurrent(10);
        // check if current is big enough (max ~20K resistor), with a 1% margin of error
        if (firstCurrent < -25 && secondCurrent > 25 && ALMOSTEQUAL(firstCurrent, secondCurrent, 0.01)) {
            result.isSuggestedType = true;
            result.orientation = Probe::combinations[i];
            // turn probes off
            Probe::combinations[i].first->turnOff();
            Probe::combinations[i].second->turnOff();
            return result;
        }
    }
    return result;
}

void Resistor::measure() {
    // set with 500K resistor to ground
    pinout.third->turnOff();
    // set as ground
    pinout.second->setVoltage(0);
    // set as VCC (1000mV)
    pinout.first->setVoltage(1000);
    // short delay
    sleep_ms(10);
    // measure
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