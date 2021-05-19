#include "bjtpnp.hpp"

DUTInformation BjtPnp::checkIfPNP() {
    DUTInformation result;
    result.isSuggestedType = false;
    for (unsigned char i = 0; i < 6; ++i) {
        // eerste pin wordt verondersteld de collector te zijn, de tweede als basis en de derde als emitter
        // zet de emitter op 1V, zodanig dat VBE = -0.7V
        Probe::combinations[i].third->setVoltage(1000);
        // zet de basis op 300mV, zodanig dat VBE = -0.7V
        Probe::combinations[i].second->setVoltage(300);
        // zet de collector op 500mV, zodanig dat VCE >= 0.2V
        Probe::combinations[i].first->setVoltage(500);
        // kort wachten
        sleep_ms(10);
        // via stroommetingen controleren indien dit om een BJT PNP gaat
        Current baseCurrent = Probe::combinations[i].second->readAverageCurrent(10);
        Current emitterCurrent = Probe::combinations[i].third->readAverageCurrent(10);
        Current collectorCurrent = Probe::combinations[i].first->readAverageCurrent(10);
        // controleren indien som collector & basis ongeveer gelijk is aan de emitter (zonder teken) en de tekens/stroomrichtingen zijn zoals verwacht
        if (ALMOSTEQUAL(ABS(baseCurrent) + ABS(collectorCurrent), emitterCurrent, .05)
        && baseCurrent > 5 && collectorCurrent > 50 && emitterCurrent < -50
        && collectorCurrent > baseCurrent) {
            result.isSuggestedType = true;
            // sommige transistoren geleiden in beide richtingen (en dus ook wanneer de collector & emitter omgedraaid zijn)
            // om zeker te zijn dat de juiste oriëntatie werd gevonden, wordt de tegengestelde oriëntatie ook gecontroleerd
            // en wordt de stroomversterking in dit geval ook gemeten
            double firstBeta = collectorCurrent / baseCurrent;
            // de derde en eerste pin worden omgedraaid, de nieuwe emitter wordt 1V
            Probe::combinations[i].first->setVoltage(1000);
            // de nieuwe collector wordt 500mV
            Probe::combinations[i].third->setVoltage(500);
            // kort wachten
            sleep_ms(10);
            // kijk of de transistor in deze oriëntatie geleid
            Current secondBaseCurrent = Probe::combinations[i].second->readAverageCurrent(10);
            if (secondBaseCurrent > 5) {
                // via de gemeten stromen de nieuwe beta bepalen
                double secondBeta = Probe::combinations[i].third->readAverageCurrent(10) / secondBaseCurrent;
                if (ABS(firstBeta) > ABS(secondBeta)) {
                    // oorspronkelijke oriëntatie leverde een grotere beta op, en was zo dus de juiste richting
                    result.orientation = Probe::combinations[i];
                    // probes afleggen
                    Probe::combinations[i].first->turnOff();
                    Probe::combinations[i].second->turnOff();
                    Probe::combinations[i].third->turnOff();
                    return result;
                } else {
                    // de nieuwe oriëntatie leverde een grotere stroomversterking op, en is zo de juiste oriëntatie
                    result.orientation = 
                    { Probe::combinations[i].third, Probe::combinations[i].second, Probe::combinations[i].first, Probe::combinations[i].thirdPinNumber, Probe::combinations[i].secondPinNumber, Probe::combinations[i].firstPinNumber };
                    // probes afleggen
                    Probe::combinations[i].first->turnOff();
                    Probe::combinations[i].second->turnOff();
                    Probe::combinations[i].third->turnOff();
                    return result;
                }
            } else {
                // de oorspronkelijke oriëntatie is de enige oriëntatie die geleidde, en is dus de juiste oriëntatie
                result.orientation = Probe::combinations[i];
                // probes afleggen
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
    while (collectorCurrent > 10 && pinout.second->currentVoltageSet < 800) {
        pinout.second->increaseVoltage();
        collectorCurrent = pinout.first->readAverageCurrent(25);
    }
}

void BjtPnp::measure() {
    unsigned int attempts = 0;
do_measure_pnp:
    // eerst wordt VBE zo klein mogelijk gezet:
    setLowestVBE();
    // korte delay
    sleep_ms(1);
    // nameten
    minVbeVoltage = (((double) pinout.second->readAverageVoltage(10)) - pinout.third->readAverageVoltage(10)) / 1000;
    pinout.second->setVoltage(pinout.second->currentVoltageSet - 25);
    sleep_ms(1);
    minBeta = 1000;
    maxBeta = 0;
    averageBeta = 0;
    double currentBeta;
    unsigned int nMeasures = 20;
    for (unsigned int i = 0; i < 20; ++i) {
        pinout.second->decreaseVoltage();
        currentBeta = ((double) pinout.first->readAverageCurrent(10)) / pinout.second->readAverageCurrent(10);
        if (currentBeta > 0) {
            if (currentBeta > maxBeta) {
                maxBeta = currentBeta;
            }
            if (currentBeta < minBeta) {
                minBeta = currentBeta;
            }
            averageBeta += currentBeta;
        } else {
            --nMeasures;
        }
    }
    averageBeta /= nMeasures;
    // kijk indien deze accuraat genoeg zijn, indien niet kan het opnieuw geprobeerd worden
    if (!(0.5 * averageBeta < minBeta && 1.5 * averageBeta > maxBeta) && attempts < 3) {
        connectionStatus = BadConnection;
        ++attempts;
        goto do_measure_pnp;
    } else if (attempts == 3) {
        connectionStatus = UnusableConnection;
    }

    pinout.first->turnOff();
    pinout.second->turnOff();
    pinout.third->turnOff();
}

void BjtPnp::generateIbIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint, bool sampleVoltage) {
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
    Voltage lowestBaseVoltage = pinout.second->currentVoltageSet;
    // vanaf hier kan de verhouding IB <-> IC gemeten worden, totdat de basisstroom of collectorstroom te groot is
    // om zo te weten met hoeveel de basisspanning moet toenemen voor elk punt

    Current baseCurrent = pinout.second->readAverageCurrent(nSamplesPerPoint);
    Current collectorCurrent = pinout.first->readAverageCurrent(nSamplesPerPoint);

    Voltage highestBaseVoltage = 150;

    pinout.second->setVoltage(lowestBaseVoltage);

    sleep_ms(2);

    MeasureResult basisMeting, collectorMeting, emitterMeting;
    basisMeting = pinout.second->doFullMeasure(nSamplesPerPoint);
    collectorMeting = pinout.first->doFullMeasure(nSamplesPerPoint);
    emitterMeting = pinout.third->doFullMeasure(nSamplesPerPoint);
    Graph::graphCurrent[0].data[0].x = basisMeting.avgA;
    Graph::graphCurrent[0].data[0].y = collectorMeting.avgA;
    Graph::graphCurrent[1].data[0].x = basisMeting.minA;
    Graph::graphCurrent[1].data[0].y = collectorMeting.minA;
    Graph::graphCurrent[2].data[0].x = basisMeting.maxA;
    Graph::graphCurrent[2].data[0].y = collectorMeting.maxA;

    Graph::minYCurrent = Graph::graphCurrent[0].data[0].y;
    Graph::maxYCurrent = Graph::graphCurrent[0].data[0].y;

    Graph::minX = Graph::graphCurrent[0].data[0].x;
    Graph::maxX = Graph::graphCurrent[0].data[0].x;

    if (sampleVoltage) {
        Graph::graphVoltage[0].data[0].x = basisMeting.avgA;
        Graph::graphVoltage[0].data[0].y = emitterMeting.avgV - collectorMeting.avgV;
        Graph::graphVoltage[1].data[0].x = basisMeting.minA;
        Graph::graphVoltage[1].data[0].y = emitterMeting.minV - collectorMeting.minV;
        Graph::graphVoltage[2].data[0].x = basisMeting.maxA;
        Graph::graphVoltage[2].data[0].y = emitterMeting.maxV - collectorMeting.maxV;

        Graph::minYVoltage = 0;
        Graph::maxYVoltage = 1.5 * Graph::graphVoltage[0].data[0].y;
    }
    UVoltage VCE = emitterMeting.avgV - collectorMeting.avgV;
    unsigned int i = 1;
    
    baseCurrent = basisMeting.avgA;
    collectorCurrent = collectorMeting.avgA;
    while (collectorCurrent > 0 && collectorCurrent < 8000 && i < nPoints) {
        pinout.second->setVoltage((((int) i) * (highestBaseVoltage - lowestBaseVoltage) / ((int) nPoints) + lowestBaseVoltage));
        while (!ALMOSTEQUAL(VCE, emitterMeting.avgV - collectorMeting.avgV, 0.05) && pinout.third->currentVoltageSet < 4000) {
            pinout.third->increaseVoltage();
            collectorMeting = pinout.first->doFullMeasure(3);
            emitterMeting = pinout.third->doFullMeasure(3);
        }

        basisMeting = pinout.second->doFullMeasure(nSamplesPerPoint);
        collectorMeting = pinout.first->doFullMeasure(nSamplesPerPoint);
        emitterMeting = pinout.third->doFullMeasure(nSamplesPerPoint);

        baseCurrent = basisMeting.avgA;
        collectorCurrent = collectorMeting.avgA;

        if (collectorCurrent < 0) {
            --i;
            break;
        }

        Graph::graphCurrent[0].data[i].x = basisMeting.avgA;
        Graph::graphCurrent[0].data[i].y = collectorMeting.avgA;
        Graph::graphCurrent[1].data[i].x = basisMeting.minA;
        Graph::graphCurrent[1].data[i].y = collectorMeting.minA;
        Graph::graphCurrent[2].data[i].x = basisMeting.maxA;
        Graph::graphCurrent[2].data[i].y = collectorMeting.maxA;

        if (sampleVoltage) {
            Graph::graphVoltage[0].data[i].x = basisMeting.avgA;
            Graph::graphVoltage[0].data[i].y = emitterMeting.avgV - collectorMeting.avgV;
            Graph::graphVoltage[1].data[i].x = basisMeting.minA;
            Graph::graphVoltage[1].data[i].y = emitterMeting.minV - collectorMeting.minV;
            Graph::graphVoltage[2].data[i].x = basisMeting.maxA;
            Graph::graphVoltage[2].data[i].y = emitterMeting.maxV - collectorMeting.maxV;
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
            if (1.5 * Graph::graphVoltage[0].data[i].y > Graph::maxYVoltage) {
                Graph::maxYVoltage = 1.5 * Graph::graphVoltage[0].data[i].y;
            }
        }
        ++i;
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

void BjtPnp::generateVceIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint) {
    // oorspronkelijke grafieken worden uit het geheugen gehaald en nieuw geheugen wordt gemaakt
    for (unsigned char i = 0; i < 3; ++i) {
        delete[] Graph::graphCurrent[i].data;
        delete[] Graph::graphVoltage[i].data;
        Graph::graphCurrent[i].data = new Point[nPoints];
        Graph::graphVoltage[i].data = nullptr;
    }

    pinout.third->setVoltage(1000);
    pinout.first->setVoltage(950);
    pinout.second->setVoltage(400);

    Current baseCurrent = pinout.second->readAverageCurrent(5);
    while (baseCurrent < 50) {
        pinout.second->increaseVoltage();
        baseCurrent = pinout.second->readAverageCurrent(5);
    }
    UVoltage beginVCE = pinout.third->readAverageVoltage(nSamplesPerPoint) - pinout.first->readAverageVoltage(nSamplesPerPoint);
    UVoltage voltageDecreasePerIteration = (600 - beginVCE) / nPoints;
    Graph::maxX = 0;
    Graph::minX = 0.5 * beginVCE;
    Graph::maxYCurrent = 0;
    MeasureResult collector, emitter;
    for (unsigned int i = 0; i < nPoints; ++i) {
        pinout.first->setVoltage(beginVCE - voltageDecreasePerIteration * i);
        baseCurrent = pinout.second->readAverageCurrent(5);
        while (baseCurrent > 55) {
            pinout.second->increaseVoltage();
            baseCurrent = pinout.second->readAverageCurrent(5);
        }
        collector = pinout.first->doFullMeasure(nSamplesPerPoint);
        emitter = pinout.third->doFullMeasure(nSamplesPerPoint);
        Graph::graphCurrent[0].data[i].x = emitter.avgV - collector.avgV;
        Graph::graphCurrent[1].data[i].x = emitter.minV - collector.minV;
        Graph::graphCurrent[2].data[i].x = emitter.maxV - collector.maxV;

        Graph::graphCurrent[0].data[i].y = collector.avgA;
        Graph::graphCurrent[1].data[i].y = collector.maxA;
        Graph::graphCurrent[2].data[i].y = collector.minA;

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

void BjtPnp::generateVbeIcGraph(unsigned int nPoints, unsigned int nSamplesPerPoint) {
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
    pinout.third->setVoltage(1000);
    pinout.first->setVoltage(400);
    pinout.second->setVoltage(600);
    // kort wachten
    sleep_ms(2);
    // aannemen dat een spanning VBE max is bij .8V, begonnen bij 400mV (1-0.600) om de grootte per stap uit te kunnen bepalen
    Voltage voltageStep = 400 / nPoints;
    // meetpunten maken
    MeasureResult base, emitter, collector;
    // elk punt overlopen voor de grafiek
    unsigned int index = 0;
    for (unsigned int i = 0; i < nPoints; ++i) {
        // nieuwe spanning instellen voor VB
        pinout.second->setVoltage(600 - (((int) i) * voltageStep));
        // VCB is constant, dus VC moet ook zoveel afnemen, met een constante offset
        pinout.first->setVoltage(400 - (((int) i) * voltageStep));
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
        Graph::graphCurrent[0].data[index].x = emitter.avgV - base.avgV;
        Graph::graphCurrent[1].data[index].x = emitter.minV - base.minV;
        Graph::graphCurrent[2].data[index].x = emitter.maxV - base.maxV;

        Graph::graphCurrent[0].data[index].y = collector.avgA;
        Graph::graphCurrent[1].data[index].y = collector.minA;
        Graph::graphCurrent[2].data[index].y = collector.maxA;
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

void BjtPnp::getPropertyText(PropertyType property, char* buffer) {
    switch (property) {
        case DESCRIPTION_LINE_1: {
            if (connectionStatus == Connected || connectionStatus == BadConnection) {
                sprintf(buffer, "Minimale beta: %f", minBeta);
            } else {
                strcpy(buffer, "De PNP BJT is te slecht aangesloten.");
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