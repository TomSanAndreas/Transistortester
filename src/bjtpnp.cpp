#include "bjtpnp.hpp"

DUTInformation BjtPnp::checkIfPNP() {
    DUTInformation result;
    result.isSuggestedType = false;
    for (unsigned char i = 0; i < 6; ++i) {
        // first pin is considered to be the collector, second as base and third as emitter
        // set emitter as 100mV, so that VBE = -0.7V
        Probe::combinations[i].third->setVoltage(1000);
        // set base as 300mV, so that VBE = -0.7V
        Probe::combinations[i].second->setVoltage(300);
        // set collector as 500mV, so that VCE >= 0.2V
        Probe::combinations[i].first->setVoltage(500);
        // wait for a short time
        sleep_ms(10);
        // check if it is a BJT using current measurements
        Current baseCurrent = Probe::combinations[i].second->readAverageCurrent(10);
        Current emitterCurrent = Probe::combinations[i].third->readAverageCurrent(10);
        Current collectorCurrent = Probe::combinations[i].first->readAverageCurrent(10);
        // check if collector + base and emitter current is similar, and the orientation of said currents
        if (ALMOSTEQUAL(ABS(baseCurrent) + ABS(collectorCurrent), emitterCurrent, .05)
        && baseCurrent > 5 && collectorCurrent > 50 && emitterCurrent < -50
        && collectorCurrent > baseCurrent) {
            result.isSuggestedType = true;
            // now we need to check the orientation: some PNP transistors will conduct in both directions (Collector -> Emitter and vice versa)
            // to make sure we have the right collector & emitter pins identified, we need to reverse the voltage on both of the pins
            // and check the beta then
            // the combination that makes for the biggest beta value identifies the right orientation
            double firstBeta = collectorCurrent / baseCurrent;
            // so now we reverse "third" and "first": first pin is considered to be the emitter, third is considered to be the collector
            // set emitter as 1000mV, so that VBE = -0.7V
            Probe::combinations[i].first->setVoltage(1000);
            // set collector as 500mV, so the transistor can conduct
            Probe::combinations[i].third->setVoltage(500);
            // wait for a short time
            sleep_ms(10);
            // check if transistor is actually conducting
            Current secondBaseCurrent = Probe::combinations[i].second->readAverageCurrent(10);
            if (secondBaseCurrent > 5) {
                // use the new currents to calculate the other beta
                double secondBeta = Probe::combinations[i].third->readAverageCurrent(10) / secondBaseCurrent;
                if (ABS(firstBeta) > ABS(secondBeta)) {
                    // the original orientation resulted in a bigger beta, so it was right the first time
                    result.orientation = Probe::combinations[i];
                    // turn off the probes
                    Probe::combinations[i].first->turnOff();
                    Probe::combinations[i].second->turnOff();
                    Probe::combinations[i].third->turnOff();
                    return result;
                } else {
                    // the new orientation resulted in a bigger beta, so collector & emitter have to be swapped
                    result.orientation = 
                    { Probe::combinations[i].third, Probe::combinations[i].second, Probe::combinations[i].first, Probe::combinations[i].thirdPinNumber, Probe::combinations[i].secondPinNumber, Probe::combinations[i].firstPinNumber };
                    // turn off the probes
                    Probe::combinations[i].first->turnOff();
                    Probe::combinations[i].second->turnOff();
                    Probe::combinations[i].third->turnOff();
                    return result;
                }
            } else {
                // the original orientation was the only one actually conducting, so this is the right orientation
                result.orientation = Probe::combinations[i];
                // turn off the probes
                Probe::combinations[i].first->turnOff();
                Probe::combinations[i].second->turnOff();
                Probe::combinations[i].third->turnOff();
                return result;
            }
        }
    }
    return result;
}

void BjtPnp::setLowestVBE() {
    // emitter is 1000mV
    pinout.third->setVoltage(1000);
    // collector is 500mV
    pinout.first->setVoltage(500);
    // basis is 300 mV, VBE = -0.7V
    pinout.second->setVoltage(300);
    Current collectorCurrent = pinout.first->readCurrent();
    // PNP heeft stroom uit de collector, dus wordt deze positief gemeten
    // VBE verlagen door B te laten stijgen, dit tot collectorcurrent klein genoeg is
    while (collectorCurrent > 1 && pinout.second->currentVoltageSet < 800) {
        pinout.second->increaseVoltage();
        collectorCurrent = pinout.first->readAverageCurrent(25);
    }
}

void BjtPnp::measure() {
    // eerst wordt VBE zo klein mogelijk gezet:
    setLowestVBE();
    // korte delay
    sleep_ms(1);
    // nameten
    minVbeVoltage = (((double) pinout.second->readAverageVoltage(10)) - pinout.third->readAverageVoltage(10)) / 1000;
    // beta kan bepaald worden via een gemiddelde van een eerste 5 meetpunten (er wordt veronderstelt dat er nog geen teken van saturatie is dan), na een kleine toename in spanning (zodat de transistor zeker ook meetbaar geleid)
    pinout.second->setVoltage(pinout.second->currentVoltageSet - 25);
    averageBeta = ((double) pinout.first->readAverageCurrent(10)) / pinout.second->readAverageCurrent(10);
    minBeta = averageBeta;        
    maxBeta = averageBeta;        
    double currentBeta;
    for (unsigned int i = 0; i < 4; ++i) {
        pinout.second->decreaseVoltage();
        currentBeta = ((double) pinout.first->readAverageCurrent(10)) / pinout.second->readAverageCurrent(10);
        if (currentBeta > maxBeta) {
            maxBeta = currentBeta;
        } else if (currentBeta < minBeta) {
            minBeta = currentBeta;
        }
        averageBeta += currentBeta;
    }
    averageBeta /= 5;
    pinout.first->turnOff();
    pinout.second->turnOff();
    pinout.third->turnOff();
}

void BjtPnp::generateIbIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint) {
    // FIXME
    // // oorspronkelijke grafieken worden uit het geheugen gehaald en nieuw geheugen wordt gemaakt
    // for (unsigned char i = 0; i < 3; ++i) {
    //     delete[] Graph::graphs[i].data;
    //     Graph::graphs[i].data = new Point[nPoints];
    // }
    // // VBE wordt zo klein mogelijk gezet
    // setLowestVBE();
    // // vanaf hier kan de verhouding IB <-> IC gemeten worden, totdat de basisstroom te groot is, de
    // // collectorstroom te groot is, of het maximum aantal punten bereikt is voor een grafiek te vormen
    // MeasureResult basisMeting, collectorMeting;
    // basisMeting = pinout.second->doFullMeasure(nSamplesPerPoint);
    // collectorMeting = pinout.first->doFullMeasure(nSamplesPerPoint);
    // Graph::graphs[0].data[0].x = basisMeting.avgA;
    // Graph::graphs[0].data[0].y = collectorMeting.avgA;
    // Graph::graphs[1].data[0].x = basisMeting.minA;
    // Graph::graphs[1].data[0].y = collectorMeting.minA;
    // Graph::graphs[2].data[0].x = basisMeting.maxA;
    // Graph::graphs[2].data[0].y = collectorMeting.maxA;
    // Graph::minX = Graph::graphs[0].data[0].x;
    // Graph::maxX = Graph::graphs[0].data[0].x;
    // Graph::minY = Graph::graphs[0].data[0].y;
    // Graph::maxY = Graph::graphs[0].data[0].y;
    // unsigned int i = 1;
    // pinout.second->increaseVoltage();
    // // FIXME(zoek het eindpunt (waar collectorcurrent 8000µA is), en deel de beginspanning VBE tot eindspanning VBE gelijk in voor nPoints)
    // Current baseCurrent = pinout.second->readAverageCurrent(nSamplesPerPoint);
    // Current collectorCurrent = pinout.first->readAverageCurrent(nSamplesPerPoint);
    // while (collectorCurrent < 8000 && i < nPoints) {
    //     basisMeting = pinout.second->doFullMeasure(nSamplesPerPoint);
    //     collectorMeting = pinout.first->doFullMeasure(nSamplesPerPoint);
    //     Graph::graphs[0].data[i].x = basisMeting.avgA;
    //     Graph::graphs[0].data[i].y = collectorMeting.avgA;
    //     Graph::graphs[1].data[i].x = basisMeting.minA;
    //     Graph::graphs[1].data[i].y = collectorMeting.minA;
    //     Graph::graphs[2].data[i].x = basisMeting.maxA;
    //     Graph::graphs[2].data[i].y = collectorMeting.maxA;
    //     if (Graph::graphs[0].data[i].x < Graph::minX) {
    //         Graph::minX = Graph::graphs[0].data[i].x;
    //     }
    //     if (Graph::graphs[0].data[i].x > Graph::maxX) {
    //         Graph::maxX = Graph::graphs[0].data[i].x;
    //     }
    //     if (Graph::graphs[0].data[i].y < Graph::minY) {
    //         Graph::minY = Graph::graphs[0].data[i].y;
    //     }
    //     if (Graph::graphs[0].data[i].y > Graph::maxY) {
    //         Graph::maxY = Graph::graphs[0].data[i].y;
    //     }
    //     while (pinout.second->readAverageCurrent(10) < baseCurrent + 2) {
    //         pinout.second->decreaseVoltage();
    //     }
    //     ++i;
    //     baseCurrent = basisMeting.avgA;
    //     collectorCurrent = collectorMeting.avgA;
    // }
    // if (i != nPoints) {
    //     for (unsigned int j = 0; j < 3; ++j) {
    //         Graph::graphs[j].data[i].x = 0;
    //         Graph::graphs[j].data[i].y = 0;
    //     }
    // }
    // Graph::graphType = GraphContext::IB_IC;
    // pinout.first->turnOff();
    // pinout.second->turnOff();
    // pinout.third->turnOff();
}

void BjtPnp::getPropertyText(PropertyType property, char* buffer) {
    switch (property) {
        case DESCRIPTION_LINE_1: {
            sprintf(buffer, "Minimale beta: %f", minBeta);
            break;
        }
        case DESCRIPTION_LINE_2: {
            sprintf(buffer, "Gemiddelde beta: %f", averageBeta);
            break;
        }
        case DESCRIPTION_LINE_3: {
            sprintf(buffer, "Maximale beta: %f", maxBeta);
            break;
        }
        case DESCRIPTION_LINE_4: {
            buffer[0] = '\0';
            break;
        }
        case PINOUT_1: {
            sprintf(buffer, "Pin %d: Collector", pinout.firstPinNumber);
            break;
        }
        case PINOUT_2: {
            sprintf(buffer, "Pin %d: Basis", pinout.secondPinNumber);
            break;
        }
        case PINOUT_3: {
            sprintf(buffer, "Pin %d: Emitter", pinout.thirdPinNumber);
            break;
        }
        case SYMBOL_NAME: {
            strcpy(buffer, "../ui/bjt_pnp.png");
            break;
        }
        case COMPONENT_NAME: {
            strcpy(buffer, "Bipolaire transistor (PNP)");
            break;
        }
        case TYPE_NAME: {
            strcpy(buffer, "BJT_PNP");
            break;
        }
        default:
            buffer[0] = '\0';
    }
}