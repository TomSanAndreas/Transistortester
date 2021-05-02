#include "bjtnpn.hpp"

DUTInformation BjtNpn::checkIfNPN() {
    DUTInformation result;
    result.isSuggestedType = false;
    for (unsigned char i = 0; i < 6; ++i) {
        // first pin is considered to be the collector, second as base and third as emitter
        // set emitter as GND, so that VBE = 0.7V
        Probe::combinations[i].third->setVoltage(0);
        // set base as 700mV, so that VBE = 0.7V
        Probe::combinations[i].second->setVoltage(700);
        // set collector as 500mV, so in case this isnt the collector, more than 700mV cant be across the actual B-E junction
        Probe::combinations[i].first->setVoltage(500);
        // wait for a short time
        sleep_ms(10);
        // check if it is a BJT using current measurements
        Current baseCurrent = Probe::combinations[i].second->readAverageCurrent(10);
        Current emitterCurrent = Probe::combinations[i].third->readAverageCurrent(10);
        Current collectorCurrent = Probe::combinations[i].first->readAverageCurrent(10);
        // check if collector + base and emitter current is similar, and the orientation of said currents
        if (ALMOSTEQUAL(ABS(baseCurrent) + ABS(collectorCurrent), emitterCurrent, .05)
        && baseCurrent < -5 && collectorCurrent < -50 && emitterCurrent > 50
        && collectorCurrent < baseCurrent) {
            result.isSuggestedType = true;
            // now we need to check the orientation: some NPN transistors will conduct in both directions (Collector -> Emitter and vice versa)
            // to make sure we have the right collector & emitter pins identified, we need to reverse the voltage on both of the pins
            // and check the beta then
            // the combination that makes for the biggest beta value identifies the right orientation
            double firstBeta = collectorCurrent / baseCurrent;
            // so now we reverse "third" and "first": first pin is considered to be the emitter, third is considered to be the collector
            // set emitter as GND, so that VBE = 0.7V
            Probe::combinations[i].first->setVoltage(0);
            // set collector as 500mV, so the transistor can conduct
            Probe::combinations[i].third->setVoltage(500);
            // wait for a short time
            sleep_ms(10);
            // check if transistor is actually conducting
            Current secondBaseCurrent = Probe::combinations[i].second->readAverageCurrent(10);
            if (secondBaseCurrent < -5) {
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

void BjtNpn::setLowestVBE() {
    // emitter is GND
    pinout.third->setVoltage(0);
    // collector is 500mV
    pinout.first->setVoltage(500);
    // basis is 710 mV
    pinout.second->setVoltage(710);
    Current collectorCurrent = pinout.first->readCurrent();
    // NPN heeft stroom in de basis, dus wordt deze als negatief gemeten
    // VBE verlagen door B te laten dalen, dit tot collectorCurrent klein genoeg is (in absolute waarde)
    while (collectorCurrent < -10 && pinout.second->currentVoltageSet > 200) {
        pinout.second->decreaseVoltage();
        collectorCurrent = pinout.first->readAverageCurrent(25);
    }
}

void BjtNpn::measure() {
    unsigned int attempts = 0;
do_measure:
    // eerst wordt VBE zo klein mogelijk gezet:
    setLowestVBE();
    // korte delay
    sleep_ms(1);
    // nameten
    minVbeVoltage = pinout.second->readAverageVoltage(10) / 1000.0;
    // beta kan bepaald worden via een gemiddelde van een eerste 5 meetpunten (er wordt veronderstelt dat er nog geen teken van saturatie is dan), na een kleine toename in spanning (zodat de transistor zeker ook meetbaar geleid)
    pinout.second->setVoltage(pinout.second->currentVoltageSet + 25);
    sleep_ms(1);
    minBeta = 1000;        
    maxBeta = 0;        
    averageBeta = 0;
    double currentBeta;
    unsigned int nMeasures = 50;
    for (unsigned int i = 0; i < 50; ++i) {
        pinout.second->increaseVoltage();
        currentBeta = ((double) pinout.first->readAverageCurrent(10)) / ((double) pinout.second->readAverageCurrent(10));
        if (currentBeta > 0) {
            if (currentBeta > maxBeta) {
                maxBeta = currentBeta;
            } else if (currentBeta < minBeta) {
                minBeta = currentBeta;
            }
            averageBeta += currentBeta;
        } else {
            --nMeasures;
        }
    }
    averageBeta /= nMeasures;
    // check if accurate enough, if not, measure again
    if (!(0.5 * averageBeta < minBeta && 1.5 * averageBeta > maxBeta) && attempts < 5) {
        connectionStatus = BadConnection;
        ++attempts;
        goto do_measure;
    } else if (attempts == 5) {
        connectionStatus = UnusableConnection;
    }

    pinout.first->turnOff();
    pinout.second->turnOff();
    pinout.third->turnOff();
}

void BjtNpn::generateIbIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint, bool sampleVoltage) {
    // oorspronkelijke grafieken worden uit het geheugen gehaald en nieuw geheugen wordt gemaakt
    for (unsigned char i = 0; i < 3; ++i) {
        delete[] Graph::graphCurrent[i].data;
        delete[] Graph::graphVoltage[i].data;
        Graph::graphCurrent[i].data = new Point[nPoints];
        if (sampleVoltage) {
            Graph::graphVoltage[i].data = new Point[nPoints];
        } else {
            Graph::graphVoltage[i].data = nullptr;
        }
    }
    // VBE wordt zo klein mogelijk gezet
    setLowestVBE();
    UVoltage lowestBaseVoltage = pinout.second->currentVoltageSet;
    // vanaf hier kan de verhouding IB <-> IC gemeten worden, totdat de basisstroom of collectorstroom te groot is
    // om zo te weten met hoeveel de basisspanning moet toenemen voor elk punt

    Current baseCurrent = pinout.second->readAverageCurrent(nSamplesPerPoint);
    Current collectorCurrent = pinout.first->readAverageCurrent(nSamplesPerPoint);

    while (collectorCurrent > - 7000 && baseCurrent > - 5000) {
        if (collectorCurrent > - 3000 && baseCurrent > - 1000) {
            pinout.second->setVoltage(pinout.second->currentVoltageSet + 20);
        } else {
            pinout.second->increaseVoltage();
        }
        baseCurrent = pinout.second->readAverageCurrent(nSamplesPerPoint);
        collectorCurrent = pinout.first->readAverageCurrent(nSamplesPerPoint);
    }

    UVoltage highestBaseVoltage = pinout.second->currentVoltageSet;

    pinout.second->setVoltage(lowestBaseVoltage);

    sleep_ms(2);

    MeasureResult basisMeting, collectorMeting, emitterMeting;
    basisMeting = pinout.second->doFullMeasure(nSamplesPerPoint);
    collectorMeting = pinout.first->doFullMeasure(nSamplesPerPoint);
    emitterMeting = pinout.third->doFullMeasure(nSamplesPerPoint);
    Graph::graphCurrent[0].data[0].x = - basisMeting.avgA;
    Graph::graphCurrent[0].data[0].y = - collectorMeting.avgA;
    Graph::graphCurrent[1].data[0].x = - basisMeting.minA;
    Graph::graphCurrent[1].data[0].y = - collectorMeting.minA;
    Graph::graphCurrent[2].data[0].x = - basisMeting.maxA;
    Graph::graphCurrent[2].data[0].y = - collectorMeting.maxA;

    Graph::minYCurrent = Graph::graphCurrent[0].data[0].y;
    Graph::maxYCurrent = Graph::graphCurrent[0].data[0].y;

    Graph::minX = Graph::graphCurrent[0].data[0].x;
    Graph::maxX = Graph::graphCurrent[0].data[0].x;

    if (sampleVoltage) {
        Graph::graphVoltage[0].data[0].x = - basisMeting.avgA;
        Graph::graphVoltage[0].data[0].y = collectorMeting.avgV - emitterMeting.avgV;
        Graph::graphVoltage[1].data[0].x = - basisMeting.minA;
        Graph::graphVoltage[1].data[0].y = collectorMeting.minV - emitterMeting.minV;
        Graph::graphVoltage[2].data[0].x = - basisMeting.maxA;
        Graph::graphVoltage[2].data[0].y = collectorMeting.maxV - emitterMeting.maxV;

        // Graph::minYVoltage = Graph::graphVoltage[0].data[0].y;
        Graph::minYVoltage = 0;
        Graph::maxYVoltage = 1.5 * Graph::graphVoltage[0].data[0].y;
    }
    UVoltage VCE = collectorMeting.avgV - emitterMeting.avgV;
    unsigned int i = 1;
    // FIXME(zoek het eindpunt (waar collectorcurrent 8000µA is), en deel de beginspanning VBE tot eindspanning VBE gelijk in voor nPoints)
    baseCurrent = pinout.second->readAverageCurrent(nSamplesPerPoint);
    collectorCurrent = pinout.first->readAverageCurrent(nSamplesPerPoint);
    while (collectorCurrent > -8000 && i < nPoints) {
        pinout.second->setVoltage((i * (highestBaseVoltage - lowestBaseVoltage) / nPoints + lowestBaseVoltage));
        while (!ALMOSTEQUAL(VCE, collectorMeting.avgV - emitterMeting.avgV, 0.05) && pinout.first->currentVoltageSet < 4000) {
            pinout.first->increaseVoltage();
            collectorMeting = pinout.first->doFullMeasure(3);
            emitterMeting = pinout.third->doFullMeasure(3);
        }

        basisMeting = pinout.second->doFullMeasure(nSamplesPerPoint);
        collectorMeting = pinout.first->doFullMeasure(nSamplesPerPoint);
        emitterMeting = pinout.third->doFullMeasure(nSamplesPerPoint);

        Graph::graphCurrent[0].data[i].x = - basisMeting.avgA;
        Graph::graphCurrent[0].data[i].y = - collectorMeting.avgA;
        Graph::graphCurrent[1].data[i].x = - basisMeting.minA;
        Graph::graphCurrent[1].data[i].y = - collectorMeting.minA;
        Graph::graphCurrent[2].data[i].x = - basisMeting.maxA;
        Graph::graphCurrent[2].data[i].y = - collectorMeting.maxA;

        if (sampleVoltage) {
            Graph::graphVoltage[0].data[i].x = - basisMeting.avgA;
            Graph::graphVoltage[0].data[i].y = collectorMeting.avgV - emitterMeting.avgV;
            Graph::graphVoltage[1].data[i].x = - basisMeting.minA;
            Graph::graphVoltage[1].data[i].y = collectorMeting.minV - emitterMeting.minV;
            Graph::graphVoltage[2].data[i].x = - basisMeting.maxA;
            Graph::graphVoltage[2].data[i].y = collectorMeting.maxV - emitterMeting.maxV;
        }

        if (Graph::graphCurrent[0].data[i].x < Graph::minX) {
            Graph::minX = Graph::graphCurrent[0].data[i].x;
        }
        if (Graph::graphCurrent[0].data[i].x > Graph::maxX) {
            Graph::maxX = Graph::graphCurrent[0].data[i].x;
        }

        if (Graph::graphCurrent[0].data[i].y < Graph::minYCurrent) {
            Graph::minYCurrent = Graph::graphCurrent[0].data[i].y;
        }
        if (Graph::graphCurrent[0].data[i].y > Graph::maxYCurrent) {
            Graph::maxYCurrent = Graph::graphCurrent[0].data[i].y;
        }

        if (sampleVoltage) {
            // if (Graph::graphVoltage[0].data[i].y < Graph::minYVoltage) {
            //     Graph::minYVoltage = Graph::graphVoltage[0].data[i].y;
            // }
            if (1.5 * Graph::graphVoltage[0].data[i].y > Graph::maxYVoltage) {
                Graph::maxYVoltage = 1.5 * Graph::graphVoltage[0].data[i].y;
            }
        }
        ++i;
        baseCurrent = basisMeting.avgA;
        collectorCurrent = collectorMeting.avgA;
    }
    if (i != nPoints) {
        for (unsigned int j = 0; j < 3; ++j) {
            Graph::graphCurrent[j].data[i].x = 0;
            Graph::graphCurrent[j].data[i].y = 0;
            if (sampleVoltage) {
                Graph::graphVoltage[j].data[i].x = 0;
                Graph::graphVoltage[j].data[i].y = 0;
            }
        }
    }
    Graph::nPoints = i;
    pinout.first->turnOff();
    pinout.second->turnOff();
    pinout.third->turnOff();
}

void BjtNpn::generateVceIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint) {
    // oorspronkelijke grafieken worden uit het geheugen gehaald en nieuw geheugen wordt gemaakt
    for (unsigned char i = 0; i < 3; ++i) {
        delete[] Graph::graphCurrent[i].data;
        delete[] Graph::graphVoltage[i].data;
        Graph::graphCurrent[i].data = new Point[nPoints];
        Graph::graphVoltage[i].data = nullptr;
    }

    pinout.third->setVoltage(0);
    pinout.first->setVoltage(50);
    pinout.second->setVoltage(600);

    Current baseCurrent = pinout.second->readAverageCurrent(5);
    while (baseCurrent > - 50) {
        pinout.second->increaseVoltage();
        baseCurrent = pinout.second->readAverageCurrent(5);
    }
    UVoltage beginVCE = pinout.first->readAverageVoltage(nSamplesPerPoint) - pinout.third->readAverageVoltage(nSamplesPerPoint);
    UVoltage voltageIncreasePerIteration = (600 - beginVCE) / nPoints;
    Graph::maxX = 0;
    Graph::minX = 0.5 * beginVCE;
    Graph::maxYCurrent = 0;
    MeasureResult collector, emitter;
    for (unsigned int i = 0; i < nPoints; ++i) {
        pinout.first->setVoltage(beginVCE + voltageIncreasePerIteration * i);
        baseCurrent = pinout.second->readAverageCurrent(5);
        while (baseCurrent < - 55) {
            pinout.second->decreaseVoltage();
            baseCurrent = pinout.second->readAverageCurrent(5);
        }
        collector = pinout.first->doFullMeasure(nSamplesPerPoint);
        emitter = pinout.third->doFullMeasure(nSamplesPerPoint);
        Graph::graphCurrent[0].data[i].x = collector.avgV - emitter.avgV;
        Graph::graphCurrent[1].data[i].x = collector.minV - emitter.minV;
        Graph::graphCurrent[2].data[i].x = collector.maxV - emitter.maxV;

        Graph::graphCurrent[0].data[i].y = - collector.avgA;
        Graph::graphCurrent[1].data[i].y = - collector.maxA;
        Graph::graphCurrent[2].data[i].y = - collector.minA;

        if (Graph::graphCurrent[0].data[i].y > Graph::maxYCurrent) {
            Graph::maxYCurrent = Graph::graphCurrent[0].data[i].y;
        } else if (Graph::graphCurrent[0].data[i].y < Graph::minYCurrent) {
            Graph::minYCurrent = Graph::graphCurrent[0].data[i].y;
        }
        if (Graph::graphCurrent[0].data[i].x > Graph::maxX) {
            Graph::maxX = Graph::graphCurrent[0].data[i].x;
        }
    }
    Graph::nPoints = nPoints;
    pinout.first->turnOff();
    pinout.second->turnOff();
    pinout.third->turnOff();
}

void BjtNpn::generateVbeIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint) {
    // oorspronkelijke grafieken worden uit het geheugen gehaald en nieuw geheugen wordt gemaakt
    for (unsigned char i = 0; i < 3; ++i) {
        delete[] Graph::graphCurrent[i].data;
        delete[] Graph::graphVoltage[i].data;
        Graph::graphCurrent[i].data = new Point[nPoints];
        Graph::graphVoltage[i].data = nullptr;
    }
    // min & max instellen op een startwaarde
    Graph::minX = 400;
    Graph::maxX = 0;
    Graph::minYCurrent = -100;
    Graph::maxYCurrent = 0;

    // alle spanningen laag (niet af!)
    pinout.third->setVoltage(0);
    pinout.first->setVoltage(900);
    pinout.second->setVoltage(400);
    // kort wachten
    sleep_ms(2);
    // aannemen dat een spanning VBE max is bij 1V, begonnen bij 400mV om de grootte per stap uit te kunnen bepalen
    UVoltage voltageStep = 600 / nPoints;
    // meetpunten maken
    MeasureResult base, emitter, collector;
    // elk punt overlopen voor de grafiek
    unsigned int index = 0;
    for (unsigned int i = 0; i < nPoints; ++i) {
        // nieuwe spanning instellen voor VB
        pinout.second->setVoltage(i * voltageStep + 400);
        // VCB is constant, dus VC moet ook zoveel toenemen, met een constante offset (bv hier 0.5 V)
        pinout.first->setVoltage(i * voltageStep + 900);
        // meten
        base = pinout.second->doFullMeasure(nSamplesPerPoint);
        collector = pinout.first->doFullMeasure(nSamplesPerPoint);
        emitter = pinout.third->doFullMeasure(nSamplesPerPoint);
        // controleren of VBE nu niet lager ligt dan het voorgaande punt (kan voorkomen indien collector stroom zorgt voor daling in spanning over BJT door toename spanning over weerstanden in de kring)
        // indien dit het geval is, wordt deze iteratie overgeslagen
        if (index > 0 && Graph::graphCurrent[0].data[index - 1].x > base.avgV - emitter.avgV) {
            continue;
        }
        // data opslaan
        Graph::graphCurrent[0].data[index].x = base.avgV - emitter.avgV;
        Graph::graphCurrent[1].data[index].x = base.minV - emitter.minV;
        Graph::graphCurrent[2].data[index].x = base.maxV - emitter.maxV;

        Graph::graphCurrent[0].data[index].y = - collector.avgA;
        Graph::graphCurrent[1].data[index].y = - collector.minA;
        Graph::graphCurrent[2].data[index].y = - collector.maxA;
        if (Graph::graphCurrent[0].data[index].y > Graph::maxYCurrent) {
            Graph::maxYCurrent = Graph::graphCurrent[0].data[index].y;
        } else if (Graph::graphCurrent[0].data[index].y < Graph::minYCurrent) {
            Graph::minYCurrent = Graph::graphCurrent[0].data[index].y;
        }
        if (Graph::graphCurrent[0].data[index].x > Graph::maxX) {
            Graph::maxX = Graph::graphCurrent[0].data[index].x;
        }
        ++index;
    }
    Graph::nPoints = index - 1;
    pinout.first->turnOff();
    pinout.second->turnOff();
    pinout.third->turnOff();
}

void BjtNpn::getPropertyText(PropertyType property, char* buffer) {
    switch (property) {
        case DESCRIPTION_LINE_1: {
            if (connectionStatus == Connected || connectionStatus == BadConnection) {
                sprintf(buffer, "Minimale beta: %f", minBeta);
            } else {
                strcpy(buffer, "De NPN BJT is te slecht aangesloten.");
            }
            break;
        }
        case DESCRIPTION_LINE_2: {
            if (connectionStatus == Connected || connectionStatus == BadConnection) {
                sprintf(buffer, "Gemiddelde beta: %f", averageBeta);
            } else {
                strcpy(buffer, "Gelieve deze opnieuw aan te sluiten");
            }
            break;
        }
        case DESCRIPTION_LINE_3: {
            if (connectionStatus == Connected || connectionStatus == BadConnection) {
                sprintf(buffer, "Maximale beta: %f", maxBeta);
            } else {
                strcpy(buffer, "en het opnieuw te proberen.");
            }
            break;
        }
        case DESCRIPTION_LINE_4: {
            if (connectionStatus == Connected || connectionStatus == UnusableConnection) {
                buffer[0] = '\0';
            } else if (connectionStatus == BadConnection) {
                strcpy(buffer, "Mogelijks is er een slechte connectie.");
            }
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
            strcpy(buffer, "../ui/bjt_npn.png");
            break;
        }
        case COMPONENT_NAME: {
            strcpy(buffer, "Bipolaire transistor (NPN)");
            break;
        }
        default:
            buffer[0] = '\0';
    }
}