#include "bjtnpn.hpp"

DUTInformation BjtNpn::checkIfNPN() {
    DUTInformation result;
    result.isSuggestedType = false;
    for (unsigned char i = 0; i < 6; ++i) {
        // eerste pin wordt als collector verondersteld, tweede als basis en de derde als emitter
        // we zetten de emitter als grond, zodanig dat VBE = 0.7V
        Probe::combinations[i].third->setVoltage(0);
        // de basispin zetten we op 700mV
        Probe::combinations[i].second->setVoltage(700);
        // de collector zetten we op 500mV, zodat, indien deze niet de collector zou zijn, niet meer dan 700mV over de werkelijke B-E junctie zou komen
        Probe::combinations[i].first->setVoltage(500);
        // kort wachten
        sleep_ms(10);
        // via metingen bepalen of oriëntatie en componenttype herkend worden
        Current baseCurrent = Probe::combinations[i].second->readAverageCurrent(10);
        Current emitterCurrent = Probe::combinations[i].third->readAverageCurrent(10);
        Current collectorCurrent = Probe::combinations[i].first->readAverageCurrent(10);
        // som van stromen basis & collector moeten gelijk zijn aan deze van emitter (zonder naar het teken te kijken), en de tekens van de stromen zelf dienen ook te kloppen
        if (ALMOSTEQUAL(ABS(baseCurrent) + ABS(collectorCurrent), emitterCurrent, .05)
        && baseCurrent < -5 && collectorCurrent < -50 && emitterCurrent > 50
        && collectorCurrent < baseCurrent) {
            result.isSuggestedType = true;
            // nu controleren we de oriëntatie zelf nog een laatste keer, aangezien sommige BJT NPN's geleiden in tegengestelde richting ook
            // indien dit het geval is, kan er in beide situaties een versterkingsfactor bepaald worden door te kijken naar de verhoudingen van de stromen
            // en wordt de oriëntatie met de grootste stroomversterking (beta) als de juiste verondersteld
            double firstBeta = collectorCurrent / baseCurrent;
            // die die nu als emitter wordt verondersteld wordt nu grond
            Probe::combinations[i].first->setVoltage(0);
            // en die die nu als collector wordt verondersteld is ook opnieuw ingesteld
            Probe::combinations[i].third->setVoltage(500);
            // kort wachten
            sleep_ms(10);
            // kijk of er stroom vloeit
            Current secondBaseCurrent = Probe::combinations[i].second->readAverageCurrent(10);
            if (secondBaseCurrent < -5) {
                // gebruik deze stromen om de nieuwe beta te bepalen
                double secondBeta = Probe::combinations[i].third->readAverageCurrent(10) / secondBaseCurrent;
                if (ABS(firstBeta) > ABS(secondBeta)) {
                    // oorspronkelijke situatie leverde een grotere stroomversterking op, dus dit was de juiste oriëntatie
                    result.orientation = Probe::combinations[i];
                    // probes afleggen
                    Probe::combinations[i].first->turnOff();
                    Probe::combinations[i].second->turnOff();
                    Probe::combinations[i].third->turnOff();
                    return result;
                } else {
                    // de nieuwe oriëntatie resulteerde in een grotere stroomversterking, dus deze is de juiste
                    result.orientation = 
                    { Probe::combinations[i].third, Probe::combinations[i].second, Probe::combinations[i].first, Probe::combinations[i].thirdPinNumber, Probe::combinations[i].secondPinNumber, Probe::combinations[i].firstPinNumber };
                    // probes afleggen
                    Probe::combinations[i].first->turnOff();
                    Probe::combinations[i].second->turnOff();
                    Probe::combinations[i].third->turnOff();
                    return result;
                }
            } else {
                // enkel de oorspronkelijke oriëntatie geleidde, en is dus de enige juiste
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
do_measure_npn:
    // eerst wordt VBE zo klein mogelijk gezet:
    setLowestVBE();
    // korte delay
    sleep_ms(1);
    // nameten
    minVbeVoltage = pinout.second->readAverageVoltage(10) / 1000.0;
    pinout.second->setVoltage(pinout.second->currentVoltageSet + 25);
    sleep_ms(1);
    minBeta = 1000;
    maxBeta = 0;
    averageBeta = 0;
    double currentBeta;
    unsigned int nMeasures = 20;
    for (unsigned int i = 0; i < 20; ++i) {
        pinout.second->increaseVoltage();
        currentBeta = ((double) pinout.first->readAverageCurrent(10)) / ((double) pinout.second->readAverageCurrent(10));
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
    // kijk indien deze accuraat genoeg zijn, indien dit niet het geval is kan het opnieuw geprobeerd worden
    if (!(0.5 * averageBeta < minBeta && 1.5 * averageBeta > maxBeta) && attempts < 3) {
        connectionStatus = BadConnection;
        ++attempts;
        goto do_measure_npn;
    } else if (attempts == 3) {
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

    while (collectorCurrent < 0 && collectorCurrent > - 5000 && baseCurrent > - 300 && pinout.second->currentVoltageSet < 900) {
        if (collectorCurrent > - 1000 && baseCurrent > - 50) {
            pinout.second->setVoltage(pinout.second->currentVoltageSet + 20);
        } else {
            pinout.second->increaseVoltage();
        }
        baseCurrent = pinout.second->readAverageCurrent(nSamplesPerPoint);
        collectorCurrent = pinout.first->readAverageCurrent(nSamplesPerPoint);
    }

    UVoltage highestBaseVoltage;
    if (collectorCurrent > 0) {
        highestBaseVoltage = pinout.second->currentVoltageSet - 10;
    } else {
        highestBaseVoltage = pinout.second->currentVoltageSet;
    }

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

        Graph::minYVoltage = 0;
        Graph::maxYVoltage = 1.5 * Graph::graphVoltage[0].data[0].y;
    }
    UVoltage VCE = collectorMeting.avgV - emitterMeting.avgV;
    unsigned int i = 1;
    
    baseCurrent = basisMeting.avgA;
    collectorCurrent = collectorMeting.avgA;
    while (collectorCurrent < 0 && collectorCurrent > -8000 && i < nPoints) {
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
        case TYPE_NAME: {
            strcpy(buffer, "BJT_NPN");
            break;
        }
        default:
            buffer[0] = '\0';
    }
}