#include "capacitor.hpp"

DUTInformation Capacitor::checkIfCapacitor() {
    DUTInformation result;
    result.isSuggestedType = false;
    // TODO
    return result;
}

void Capacitor::measure() {
    // charge capacitor
    pinout.third->turnOff();
    pinout.second->setVoltage(0);
    pinout.first->setVoltage(500);
    Current result = pinout.second->readAverageCurrent(10);
    unsigned int i = 0;
    while (result > 20 && i < 50) {
        sleep_ms(10);
        result = pinout.second->readAverageCurrent(10);
        ++i;
    }
    if (i != 50 && i != 0) {
        // if i is between 0 and 50, it is safe to assume a capacitor was actually connected
        // reset i
        i = 0;
        // discharge capacitor by setting probe to 0V
        pinout.first->setVoltage(0);
        result = pinout.second->readCurrent();
        while (result > 5) {
            ++i;
            result = pinout.second->readCurrent();
        }
        // if i is too small, it is possible the connected capacitor is very small in capacitance,
        // so it needs to be measured again, but its discharge has to be done with the bigger built-in
        // resistors from the DAC
        if (i < 5) {
            // TODO
        }
        // i is big enough, so the capacitance of the capacitor can be determined
        else {
            // TODO
        }
    }
}

void Capacitor::getPropertyText(PropertyType property, char* buffer) {
    switch (property) {
        case DESCRIPTION_LINE_1: {
            sprintf(buffer, "Gemeten capaciteit: %f", capacitance);
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
            strcpy(buffer, "../ui/capacitor.png");
            break;
        }
        case COMPONENT_NAME: {
            strcpy(buffer, "Condensator");
            break;
        }
        case TYPE_NAME: {
            strcpy(buffer, "CAPACITOR");
            break;
        }
        default:
            buffer[0] = '\0';
    }
}