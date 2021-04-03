#include "user_interface.hpp"
#include <thread>

#ifndef WINDOWS
#include <X11/Xlib.h>
#endif

// determine if both values are at least almost equal, only percentage% different max
#define ALMOSTEQUAL(value1, value2, percentage) (ABS(ABS(value1) - ABS(value2)) < percentage * ABS(value1))

void calibrate();
void destroy();
void determineType();
// flags used in the various threads
bool determenRequested = false, measurementRequested = false, programAlive = true, calibrated = false;

struct ProbeCombination {
    Probe* first,* second,* third;
    unsigned int firstPinNumber, secondPinNumber, thirdPinNumber;
    static ProbeCombination* possibleCombinations;
};

struct Point {
    float x, y;
};

struct Graph {
    Point data[50];
    static unsigned int graphType;
    static int maxX, maxY, minX, minY;
};

unsigned int Graph::graphType;

struct GraphContext {
    const char* xUnit,* yUnit;
    const char* graphTitle;
    const char* buttonDiscription[3];
    static const GraphContext data[];
    static const unsigned int IB_IC, IC_VCE, IC_VBE;
};

const unsigned int GraphContext::IB_IC = 0;
const unsigned int GraphContext::IC_VCE = 1;
const unsigned int GraphContext::IC_VBE = 2;

const GraphContext GraphContext::data[] = {
    {"uA", "mA", "IC i.f.v. IB",  {"IC i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}},
    {"mA", "mV", "VCE i.f.v. IC", {"IC i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}},
    {"mA", "mV", "VBE i.f.v. IC", {"IC i.f.v. IB", "VCE i.f.v. IC", "VBE i.f.v. IC"}}
};

int Graph::maxX;
int Graph::maxY;
int Graph::minX;
int Graph::minY;

Graph graphs[3];

ProbeCombination* ProbeCombination::possibleCombinations;

enum ComponentType {
    RESISTOR       = 0,
    CAPACITOR      = 1,
    DIODE          = 2,
    BJT_NPN        = 3,
    BJT_PNP        = 4,
    MOSFET_NMOS    = 5,
    MOSFET_PMOS    = 6,
    MOSFET_JFET    = 7,
    UNKNOWN_DEVICE = 8
};

struct ResistorData {
    double resistance;
    ProbeCombination connectedPins;
    void measure() {
        // set with 500K resistor to ground
        connectedPins.third->turnOff();
        // set as ground
        connectedPins.second->setVoltage(0);
        // set as VCC (500mV)
        connectedPins.first->setVoltage(500);
        // short delay
        sleep_ms(10);
        // measure
        MeasureResult result1 = connectedPins.first->doFullMeasure(10);
        MeasureResult result2 = connectedPins.second->doFullMeasure(10);
        resistance = ((double) (result1.avgV - result2.avgV)) * 1000 / result2.avgA;
        connectedPins.first->turnOff();
        connectedPins.second->turnOff();
    }
};

struct CapacitorData {
    double capacitance;
    ProbeCombination connectedPins;
    void measure() {
        // charge capacitor
        connectedPins.third->turnOff();
        connectedPins.second->setVoltage(0);
        connectedPins.first->setVoltage(500);
        Current result = connectedPins.second->readAverageCurrent(10);
        unsigned int i = 0;
        while (result > 20 && i < 50) {
            sleep_ms(10);
            result = connectedPins.second->readAverageCurrent(10);
            ++i;
        }
        if (i != 50 && i != 0) {
            // if i is between 0 and 50, it is safe to assume a capacitor was actually connected
            // reset i
            i = 0;
            // discharge capacitor by setting probe to 0V
            connectedPins.first->setVoltage(0);
            result = connectedPins.second->readCurrent();
            while (result > 5) {
                ++i;
                result = connectedPins.second->readCurrent();
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
};

struct DiodeData {
    UVoltage voltageDrop;
    ProbeCombination connectedPins;
    void measure() {
        connectedPins.third->turnOff();
        connectedPins.second->setVoltage(0);
        connectedPins.first->setVoltage(650);
        Current result = connectedPins.second->readAverageCurrent(10);
        voltageDrop = connectedPins.first->currentVoltageSet;
        while (result < 5 && voltageDrop < 800) {
            connectedPins.first->increaseVoltage();
            result = connectedPins.first->readAverageCurrent(10);
        }
        voltageDrop = connectedPins.first->readAverageVoltage(10) - connectedPins.second->readAverageVoltage(10);
        connectedPins.first->turnOff();
        connectedPins.second->turnOff();
    }
};

struct BjtNpnData {
    double averageBeta;
    double minBeta;
    double maxBeta;
    ProbeCombination collectorBaseEmitterPins;
    // "init" de transistor, m.a.w. zet VBE zo klein mogelijk, zodat metingen kunnen worden gedaan 
    void init() {
        // emitter is GND
        collectorBaseEmitterPins.third->setVoltage(0);
        // collector is 500mV
        collectorBaseEmitterPins.first->setVoltage(500);
        // basis is 710 mV
        collectorBaseEmitterPins.second->setVoltage(710);
        Current collectorCurrent = collectorBaseEmitterPins.first->readCurrent();
        // NPN heeft stroom in de basis, dus wordt deze als negatief gemeten
        // VBE verlagen door B te laten dalen, dit tot baseCurrent klein genoeg is (in absolute waarde)
        while (collectorCurrent < -1 && collectorBaseEmitterPins.second->currentVoltageSet > 200) {
            collectorBaseEmitterPins.second->decreaseVoltage();
            collectorCurrent = collectorBaseEmitterPins.first->readAverageCurrent(25);
        }
    }
    // "destroy" de meetsetup, door alle gebruikte pinnen af te leggen
    void destroy() {
        collectorBaseEmitterPins.first->turnOff();
        collectorBaseEmitterPins.second->turnOff();
        collectorBaseEmitterPins.third->turnOff();
    }
    void measure() {
        // eerst wordt VBE zo klein mogelijk gezet:
        init();
        // beta kan bepaald worden via een gemiddelde van een eerste 5 meetpunten (er wordt veronderstelt dat er nog geen teken van saturatie is dan)
        collectorBaseEmitterPins.second->increaseVoltage();
        averageBeta = ((double) collectorBaseEmitterPins.first->readAverageCurrent(10)) / collectorBaseEmitterPins.second->readAverageCurrent(10);
        minBeta = averageBeta;        
        maxBeta = averageBeta;        
        double currentBeta;
        for (unsigned int i = 0; i < 5; ++i) {
            collectorBaseEmitterPins.second->increaseVoltage();
            currentBeta = ((double) collectorBaseEmitterPins.first->readAverageCurrent(10)) / collectorBaseEmitterPins.second->readAverageCurrent(10);
            if (currentBeta > maxBeta) {
                maxBeta = currentBeta;
            } else if (currentBeta < minBeta) {
                minBeta = currentBeta;
            }
            averageBeta += currentBeta;
        }
        averageBeta /= 5;
        destroy();
    }
    void graphIbIc() {
        // eerst wordt VBE zo klein mogelijk gezet
        init();
        // vanaf hier kan de verhouding IB <-> IC gemeten worden, totdat de basisstroom te hoog is, voor een grafiek te vormen
        MeasureResult basisMeting, collectorMeting;
        basisMeting = collectorBaseEmitterPins.second->doFullMeasure(10);
        collectorMeting = collectorBaseEmitterPins.first->doFullMeasure(10);
        graphs[0].data[0].x = - basisMeting.avgA;
        graphs[0].data[0].y = - collectorMeting.avgA;
        graphs[1].data[0].x = - basisMeting.minA;
        graphs[1].data[0].y = - collectorMeting.minA;
        graphs[2].data[0].x = - basisMeting.maxA;
        graphs[2].data[0].y = - collectorMeting.maxA;
        Graph::minX = graphs[0].data[0].x;
        Graph::maxX = graphs[0].data[0].x;
        Graph::minY = graphs[0].data[0].y;
        Graph::maxY = graphs[0].data[0].y;
        unsigned int i = 1;
        collectorBaseEmitterPins.second->increaseVoltage();
        Current baseCurrent = collectorBaseEmitterPins.second->readAverageCurrent(10);
        Current collectorCurrent = collectorBaseEmitterPins.first->readAverageCurrent(10);
        while (collectorCurrent > -8000 && i < 50) {
            basisMeting = collectorBaseEmitterPins.second->doFullMeasure(10);
            collectorMeting = collectorBaseEmitterPins.first->doFullMeasure(10);
            graphs[0].data[i].x = - basisMeting.avgA;
            graphs[0].data[i].y = - collectorMeting.avgA;
            graphs[1].data[i].x = - basisMeting.minA;
            graphs[1].data[i].y = - collectorMeting.minA;
            graphs[2].data[i].x = - basisMeting.maxA;
            graphs[2].data[i].y = - collectorMeting.maxA;
            if (graphs[0].data[i].x < Graph::minX) {
                Graph::minX = graphs[0].data[i].x;
            }
            if (graphs[0].data[i].x > Graph::maxX) {
                Graph::maxX = graphs[0].data[i].x;
            }
            if (graphs[0].data[i].y < Graph::minY) {
                Graph::minY = graphs[0].data[i].y;
            }
            if (graphs[0].data[i].y > Graph::maxY) {
                Graph::maxY = graphs[0].data[i].y;
            }
            while (collectorBaseEmitterPins.second->readAverageCurrent(10) > baseCurrent - 2) {
                collectorBaseEmitterPins.second->increaseVoltage();
            }
            ++i;
            baseCurrent = basisMeting.avgA;
            collectorCurrent = collectorMeting.avgA;
        }
        if (i != 50) {
            for (unsigned int j = 0; j < 3; ++j) {
                graphs[j].data[i].x = 0;
                graphs[j].data[i].y = 0;
            }
        }
        Graph::graphType = GraphContext::IB_IC;
        destroy();
    }
};

struct BjtPnpData {
    double averageBeta;
    double minBeta;
    double maxBeta;
    ProbeCombination collectorBaseEmitterPins;
};

struct MosfetNmosData {
    double beta;
    unsigned int saturationVoltageSourceDrain;
    unsigned int saturationVoltageGateDrain;
    ProbeCombination gateDrainPins;
    ProbeCombination sourceDrainPins;
    ProbeCombination sourceGatePins;
};

struct MosfetPmosData {
    double beta;
    unsigned int saturationVoltageSourceDrain;
    unsigned int saturationVoltageGateDrain;
    ProbeCombination gateDrainPins;
    ProbeCombination sourceDrainPins;
    ProbeCombination sourceGatePins;
};

struct MosfetJfetData {
    double beta;
    unsigned int saturationVoltageSourceDrain;
    unsigned int saturationVoltageGateDrain;
    ProbeCombination gateDrainPins;
    ProbeCombination sourceDrainPins;
    ProbeCombination sourceGatePins;
};

union ComponentData {
    ResistorData resistorData;
    CapacitorData capacitorData;
    DiodeData diodeData;
    BjtNpnData bjtNpnData;
    BjtPnpData bjtPnpData;
    MosfetNmosData mosfetNmosData;
    MosfetPmosData mosfetPmosData;
    MosfetJfetData mosfetJfetData;
};

struct Component {
    ComponentType type;
    ComponentData data;
} currentComponent;

struct CalibrationDialog {
    GtkDialog* self;
    GtkButton* dialog_close_button,* start_button;
    GtkProgressBar* progress_bar;
};

// everything in this struct is static, so the lambda can have a reference
// to its members without needing "this" in its capture list
// there will always be only 1 GraphWindow visible, so it doesn't change
// the rest of the functionality
struct GraphWindow {
    static GtkWidget* self;
    static GtkLabel* yLabelsLeft[9],* yLabelsRight[9];
    static const char* yLabelsLeftNames[],* yLabelsRightNames[];
    static GtkLabel* xLabels[21];
    static const char* xLabelsNames[];
    static GtkLabel* unit0,* unit1,* unit2,* unit3;
    static GtkLabel* graphTitle;
    void makeCompletelyInvisible() {
        // uitvoeren op de "main" thread; om deze lambda uitvoerbaar te maken met een lege capture-list, zijn alle objecten static
        g_idle_add(G_SOURCE_FUNC(+[]() {
            // zet alle x-labels onzichtbaar
            for (unsigned int i = 0; i < 21; ++i) {
                gtk_widget_set_opacity((GtkWidget*) xLabels[i], 0.0);
            }
            // zet alle y-labels onzichtbaar
            for (unsigned int i = 0; i < 9; ++i) {
                gtk_widget_set_opacity((GtkWidget*) yLabelsLeft[i], 0.0);
                gtk_widget_set_opacity((GtkWidget*) yLabelsRight[i], 0.0);
            }
            // zet de eenheden op het einde van de X- en Y-as onzichtbaar
            gtk_widget_set_opacity((GtkWidget*) unit1, 0.0);
            gtk_widget_set_opacity((GtkWidget*) unit2, 0.0);
            // zet de titel onzichtbaar
            gtk_widget_set_opacity((GtkWidget*) graphTitle, 0.0);
            // grafiek onzichtbaar maken
            gtk_widget_set_opacity(self, 0.0);
            return FALSE;
        }), NULL);
    }
    void updateGraph() {
        // uitvoeren op de "main" thread; om deze lambda uitvoerbaar te maken met een lege capture-list, zijn alle objecten static
        g_idle_add(G_SOURCE_FUNC(+[]() {
            // zet alle x-labels zichtbaar
            for (unsigned int i = 0; i < 21; ++i) {
                gtk_widget_set_opacity((GtkWidget*) xLabels[i], 1.0);
            }
            // zet alle y-labels zichtbaar
            for (unsigned int i = 0; i < 9; ++i) {
                gtk_widget_set_opacity((GtkWidget*) yLabelsLeft[i], 1.0);
                // gtk_widget_set_opacity((GtkWidget*) yLabelsRight[i], 1.0);
            }
            // zet de eenheden op het einde van de X- en Y-as zichtbaar
            gtk_widget_set_opacity((GtkWidget*) unit1, 1.0);
            gtk_widget_set_opacity((GtkWidget*) unit2, 1.0);
            // stel de eenheden op het einde van de X- en Y-as in
            gtk_label_set_text(unit1, GraphContext::data[Graph::graphType].xUnit);
            gtk_label_set_text(unit2, GraphContext::data[Graph::graphType].yUnit);
            // zet de titel zichtbaar
            gtk_widget_set_opacity((GtkWidget*) graphTitle, 1.0);
            // stel de titel in
            gtk_label_set_text(graphTitle, GraphContext::data[Graph::graphType].graphTitle);
            // grafiek zichtbaar maken
            gtk_widget_set_opacity(self, 1.0);
            // grafiek opnieuw tekenen
            gtk_widget_queue_draw(self);
            // labels zetten
            char buffer[10];
            for (unsigned char i = 0; i < 21; ++i) {
                sprintf(buffer, "%4d%*s", (i + 1) * (Graph::maxX - Graph::minX) / 21 + Graph::minX, 2, " ");
                gtk_label_set_text(xLabels[i], buffer);
            }
            for (unsigned char i = 0; i < 9; ++i) {
                sprintf(buffer, "%8d", (i + 1) * (Graph::maxY - Graph::minY) / 9 + Graph::minY);
                gtk_label_set_text(yLabelsLeft[i], buffer);
            }
            return FALSE;
        }), NULL);
    }
};

GtkWidget* GraphWindow::self;
GtkLabel* GraphWindow::yLabelsLeft[9],* GraphWindow::yLabelsRight[9],* GraphWindow::xLabels[21],* GraphWindow::unit0,* GraphWindow::unit1,* GraphWindow::unit2,* GraphWindow::unit3,* GraphWindow::graphTitle;

const char* GraphWindow::yLabelsLeftNames[] = {
    "y_0_0", "y_0_1", "y_0_2", "y_0_3", "y_0_4", "y_0_5", "y_0_6", "y_0_7", "y_0_8"
};

const char* GraphWindow::yLabelsRightNames[] = {
    "y_1_0", "y_1_1", "y_1_2", "y_1_3", "y_1_4", "y_1_5", "y_1_6", "y_1_7", "y_1_8"
};

const char* GraphWindow::xLabelsNames[] = {
    "x_0", "x_1", "x_2", "x_3", "x_4", "x_5", "x_6", "x_7", "x_8", "x_9", "x_10", "x_11", "x_12", "x_13", "x_14", "x_15", "x_16", "x_17", "x_18", "x_19", "x_20"
};

// de buttons zijn static, zodanig dat de lambda "this" niet in de capture-list moet hebben
struct BottomPanel {
    static GtkButton* toggle1,* toggle2,* toggle3;
    GraphWindow graphWindow;
    void disableButtons() {
        // uitvoeren op "main" thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            // buttons onklikbaar maken
            gtk_widget_set_sensitive((GtkWidget*) toggle1, FALSE);
            gtk_widget_set_sensitive((GtkWidget*) toggle2, FALSE);
            gtk_widget_set_sensitive((GtkWidget*) toggle3, FALSE);
            // tekst instellen
            gtk_button_set_label(toggle1, "Niet beschikbaar");
            gtk_button_set_label(toggle2, "Niet beschikbaar");
            gtk_button_set_label(toggle3, "Niet beschikbaar");
            return FALSE;
        }), NULL);
    }
    void updateButtons() {
        // uitvoeren op "main" thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            // buttons klikbaar maken
            gtk_widget_set_sensitive((GtkWidget*) toggle1, TRUE);
            gtk_widget_set_sensitive((GtkWidget*) toggle2, TRUE);
            gtk_widget_set_sensitive((GtkWidget*) toggle3, TRUE);
            // tekst instellen
            gtk_button_set_label(toggle1, GraphContext::data[Graph::graphType].buttonDiscription[0]);
            gtk_button_set_label(toggle2, GraphContext::data[Graph::graphType].buttonDiscription[1]);
            gtk_button_set_label(toggle3, GraphContext::data[Graph::graphType].buttonDiscription[2]);
            return FALSE;
        }), NULL);
    }
};

GtkButton* BottomPanel::toggle1,* BottomPanel::toggle2,* BottomPanel::toggle3;

struct MeasureProperties {
    GtkLabel* description[3];
    static const char* descriptionNames[];
    GtkLabel* currentValue[3];
    static const char* currentValueNames[];
    GtkButton* decrementValue[3];
    static const char* decrementValueNames[];
    GtkButton* incrementValue[3];
    static const char* incrementValueNames[];
};

const char* MeasureProperties::descriptionNames[] = {
    "samples_label", "punten_label", "other_label"
};

const char* MeasureProperties::currentValueNames[] = {
    "samples_counter", "punten_counter", "other_counter"
};

const char* MeasureProperties::decrementValueNames[] = {
    "samples_decrement", "punten_decrement", "other_decrement"
};

const char* MeasureProperties::incrementValueNames[] = {
    "samples_increment", "punten_increment", "other_increment"
};

struct ComponentProperties {
    static GtkLabel* property[4];
    static const char* propertyNames[];
    static inline void setInvisible() {
        g_idle_add(G_SOURCE_FUNC(+[]() {
            gtk_widget_set_opacity((GtkWidget*) property[0], 0.0);
            gtk_widget_set_opacity((GtkWidget*) property[1], 0.0);
            gtk_widget_set_opacity((GtkWidget*) property[2], 0.0);
            gtk_widget_set_opacity((GtkWidget*) property[3], 0.0);
            return FALSE;
        }), NULL);
    }
    static inline void setVisible() {
        g_idle_add(G_SOURCE_FUNC(+[]() {
            gtk_widget_set_opacity((GtkWidget*) property[0], 1.0);
            gtk_widget_set_opacity((GtkWidget*) property[1], 1.0);
            gtk_widget_set_opacity((GtkWidget*) property[2], 1.0);
            gtk_widget_set_opacity((GtkWidget*) property[3], 1.0);
            return FALSE;
        }), NULL);
    }
    static void update() {
        // main thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            char buffer[35];
            switch (currentComponent.type) {
                case RESISTOR: {
                    setVisible();
                    sprintf(buffer, "Weerstandswaarde: %f", currentComponent.data.resistorData.resistance);
                    gtk_label_set_text(property[0], buffer);
                    break;
                }
                case CAPACITOR: {
                    setVisible();
                    sprintf(buffer, "Capaciteit: %f", currentComponent.data.capacitorData.capacitance);
                    gtk_label_set_text(property[0], buffer);
                    break;
                }
                case DIODE: {
                    setVisible();
                    sprintf(buffer, "Spanningsval: %f", ((double) currentComponent.data.diodeData.voltageDrop) / 1000);
                    gtk_label_set_text(property[0], buffer);
                    break;
                }
                case BJT_NPN: {
                    setVisible();
                    sprintf(buffer, "Minimum beta: %f", ((double) currentComponent.data.bjtNpnData.minBeta) / 1000);
                    gtk_label_set_text(property[0], buffer);
                    sprintf(buffer, "Gemiddelde beta: %f", ((double) currentComponent.data.bjtNpnData.averageBeta) / 1000);
                    gtk_label_set_text(property[1], buffer);
                    sprintf(buffer, "Maximum beta: %f", ((double) currentComponent.data.bjtNpnData.maxBeta) / 1000);
                    gtk_label_set_text(property[2], buffer);
                    break;
                }
                case BJT_PNP: {
                    setVisible();
                    break;
                }
                case MOSFET_NMOS: {
                    setVisible();
                    break;
                }
                case MOSFET_PMOS: {
                    setVisible();
                    break;
                }
                case MOSFET_JFET: {
                    setVisible();
                    break;
                }
                case UNKNOWN_DEVICE:
                default: {
                    setInvisible();
                    break;
                }
            }
            return FALSE;
        }), NULL);
    }
};

GtkLabel* ComponentProperties::property[4];

const char* ComponentProperties::propertyNames[] = {
    "property_0", "property_1", "property_2", "property_3"
};

struct ComponentLayout {
    static GtkLabel* pinout[3];
    static const char* pinoutNames[];
    static GtkImage* symbol;
    static GtkLabel* componentName;
    static const char* possibleComponentNames[];
    static void update() {
        // updaten op main thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            gtk_label_set_text(componentName, ComponentLayout::possibleComponentNames[currentComponent.type]);
            char buffer[15];
            switch (currentComponent.type) {
                case RESISTOR: {
                    gtk_image_set_from_file(symbol, "../ui/resistor.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 0.0);
                    sprintf(buffer, "Pin %d", currentComponent.data.resistorData.connectedPins.firstPinNumber);
                    gtk_label_set_text(pinout[0], buffer);
                    sprintf(buffer, "Pin %d", currentComponent.data.resistorData.connectedPins.secondPinNumber);
                    gtk_label_set_text(pinout[2], buffer);
                    break;
                }
                case CAPACITOR: {
                    gtk_image_set_from_file(symbol, "../ui/capacitor.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 0.0);
                    sprintf(buffer, "Pin %d", currentComponent.data.capacitorData.connectedPins.firstPinNumber);
                    gtk_label_set_text(pinout[0], buffer);
                    sprintf(buffer, "Pin %d", currentComponent.data.capacitorData.connectedPins.secondPinNumber);
                    gtk_label_set_text(pinout[2], buffer);
                    break;
                }
                case DIODE: {
                    gtk_image_set_from_file(symbol, "../ui/diode.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 0.0);
                    sprintf(buffer, "Pin %d", currentComponent.data.diodeData.connectedPins.firstPinNumber);
                    gtk_label_set_text(pinout[0], buffer);
                    sprintf(buffer, "Pin %d", currentComponent.data.diodeData.connectedPins.secondPinNumber);
                    gtk_label_set_text(pinout[2], buffer);
                    break;
                }
                case BJT_NPN: {
                    gtk_image_set_from_file(symbol, "../ui/bjt_npn.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 1.0);
                    sprintf(buffer, "Pin %d: Collector", currentComponent.data.bjtNpnData.collectorBaseEmitterPins.firstPinNumber);
                    gtk_label_set_text(pinout[0], buffer);
                    sprintf(buffer, "Pin %d: Basis", currentComponent.data.bjtNpnData.collectorBaseEmitterPins.secondPinNumber);
                    gtk_label_set_text(pinout[1], buffer);
                    sprintf(buffer, "Pin %d: Emitter", currentComponent.data.bjtNpnData.collectorBaseEmitterPins.thirdPinNumber);
                    gtk_label_set_text(pinout[2], buffer);
                    break;
                }
                case BJT_PNP: {
                    gtk_image_set_from_file(symbol, "../ui/bjt_pnp.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 1.0);
                    sprintf(buffer, "Pin %d: Collector", currentComponent.data.bjtPnpData.collectorBaseEmitterPins.firstPinNumber);
                    gtk_label_set_text(pinout[0], buffer);
                    sprintf(buffer, "Pin %d: Basis", currentComponent.data.bjtPnpData.collectorBaseEmitterPins.secondPinNumber);
                    gtk_label_set_text(pinout[1], buffer);
                    sprintf(buffer, "Pin %d: Emitter", currentComponent.data.bjtPnpData.collectorBaseEmitterPins.thirdPinNumber);
                    gtk_label_set_text(pinout[2], buffer);
                    break;
                }
                case MOSFET_NMOS: {
                    // FIXME
                    gtk_image_set_from_file(symbol, "../ui/unknown.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 1.0);
                    break;
                }
                case MOSFET_PMOS: {
                    // FIXME
                    gtk_image_set_from_file(symbol, "../ui/unknown.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 1.0);
                    break;
                }
                case MOSFET_JFET: {
                    // FIXME
                    gtk_image_set_from_file(symbol, "../ui/unknown.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 1.0);
                    break;
                }
                case UNKNOWN_DEVICE:
                default: {
                    gtk_image_set_from_file(symbol, "../ui/unknown.png");
                    gtk_widget_set_opacity((GtkWidget*) pinout[1], 1.0);
                    gtk_label_set_text(pinout[0], "Pin 1: n.v.t.");
                    gtk_label_set_text(pinout[1], "Pin 2: n.v.t.");
                    gtk_label_set_text(pinout[2], "Pin 3: n.v.t.");
                    break;
                }
            }
            return FALSE;
        }), NULL);
    }
};

GtkLabel* ComponentLayout::pinout[3];
GtkImage* ComponentLayout::symbol;
GtkLabel* ComponentLayout::componentName;

const char* ComponentLayout::pinoutNames[] = {
    "pin_1_name", "pin_2_name", "pin_3_name"
};

const char* ComponentLayout::possibleComponentNames[] = {
    "Weerstand", "Condensator", "Diode", "Bipolaire Transistor (NPN)", "Bipolaire Transistor (PNP)", "MOSFET (NMOS)", "MOSFET (PMOS)", "MOSFET (PMOS)", "Ongekend apparaat"
};

struct TopPanel {
    GtkButton* startButton;
    MeasureProperties settings;
    ComponentProperties properties;
    ComponentLayout component;
    GtkButton* help;
};

struct MainWindow {
    TopPanel topPanel;
    BottomPanel bottomPanel;
} mainWindow;

GdkRGBA colors[3] { {.35, .5, .4, 1}, {.6, .2, .45, 1}, {.6, .5, .45, 1} };
CalibrationDialog calibrationDialog;

std::thread calibrationThread, measurementThread;

extern "C" {
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void destroy_signal(GtkWidget* w, gpointer user_data) {
        destroy();
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    gboolean draw_signal(GtkWidget* widget, cairo_t* cr, gpointer data) {
        // afmetingen grafiek in pixels bepalen
        unsigned int width = gtk_widget_get_allocated_width(widget);
        unsigned int height = gtk_widget_get_allocated_height(widget);
        // schalen berekenen
        double scaleX, scaleY;
        scaleX = ((double) width) / (Graph::maxX - Graph::minX);
        scaleY = ((double) height) / (Graph::maxY - Graph::minY);
        // raster tekenen: 21 verticale lijnen en 9 horizontale lijnen op gelijke afstand
        cairo_set_source_rgba(cr, 0, 0, 0, .1);
        unsigned int nPixelsVertical = height / 9;
        for (unsigned int i = nPixelsVertical >> 1; i < 11 * nPixelsVertical; i += nPixelsVertical) {
            cairo_line_to(cr, 0.01 * width, i);
            cairo_line_to(cr, 0.99 * width, i);
            cairo_stroke(cr);
        }
        unsigned int nPixelsHorizontal = width / 21;
        for (unsigned int i = nPixelsHorizontal >> 1; i < 23 * nPixelsHorizontal; i += nPixelsHorizontal) {
            cairo_line_to(cr, i, 0.01 * height);
            cairo_line_to(cr, i, 0.99 * height);
            cairo_stroke(cr);
        }
        // plotten grafieken
        for (unsigned char i = 0; i < 3; ++i) {
            for (unsigned int j = 0; j < 50; ++j) {
		if (graphs[i].data[j].x == 0 && graphs[i].data[j].y == 0)
			break;
                cairo_line_to(cr, (graphs[i].data[j].x - Graph::minX) * scaleX, height - (graphs[i].data[j].y - Graph::minY) * scaleY);
            }
            gdk_cairo_set_source_rgba(cr, &colors[i]);
            cairo_stroke(cr);
        }
        return false;
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void start_calibration(GtkWidget* widget, gpointer user_data) {
        calibrate();
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void close_dialog(GtkWidget* widget, gpointer user_data) {
        gtk_widget_destroy((GtkWidget*) calibrationDialog.self);
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void determine_type(GtkWidget* widget, gpointer user_data) {
        determenRequested = true;
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void measure(GtkWidget* widget, gpointer user_data) {
        measurementRequested = true;
    }
}

void determineType() {
    // check if DUT is a resistor
    for (unsigned char i = 0; i < 3; ++i) {
        // turn off the third probe
        ProbeCombination::possibleCombinations[i].third->turnOff();
        // set the second probe as GND
        ProbeCombination::possibleCombinations[i].second->setVoltage(0);
        // set the first probe as VCC (500 mV)
        ProbeCombination::possibleCombinations[i].first->setVoltage(500);
        // wait for a short time
        sleep_ms(10);
        // check if a current can be measured in first & second probe
        MeasureResult results1 = ProbeCombination::possibleCombinations[i].first->doFullMeasure(10);
        MeasureResult results2 = ProbeCombination::possibleCombinations[i].second->doFullMeasure(10);
        // check if current is big enough (max ~20K resistor), with a 1% margin of error
        if (((results1.avgA < -25 && results2.avgA > 25) || (results1.avgA > 25 && results2.avgA < -25)) && ALMOSTEQUAL(results2.avgA, results1.avgA, 0.01)) {
            currentComponent.type = ComponentType::RESISTOR;
            currentComponent.data.resistorData.connectedPins = ProbeCombination::possibleCombinations[i];
            return;
        }
    }
    // check if DUT is a capacitor
    //TODO
    // check if DUT is a BJT NPN transistor
    for (unsigned char i = 0; i < 6; ++i) {
        // first pin is considered to be the collector, second as base and third as emitter
        // set emitter as GND, so that VBE = 0.7V
        ProbeCombination::possibleCombinations[i].third->setVoltage(0);
        // set base as 700mV, so that VBE = 0.7V
        ProbeCombination::possibleCombinations[i].second->setVoltage(700);
        // set collector as 500mV, so in case this isnt the collector, more than 700mV cant be across the actual B-E junction
        ProbeCombination::possibleCombinations[i].first->setVoltage(500);
        // wait for a short time
        sleep_ms(10);
        // check if it is a BJT using current measurements
        Current baseCurrent = ProbeCombination::possibleCombinations[i].second->readAverageCurrent(10);
        Current emitterCurrent = ProbeCombination::possibleCombinations[i].third->readAverageCurrent(10);
        Current collectorCurrent = ProbeCombination::possibleCombinations[i].first->readAverageCurrent(10);
        // check if collector + base and emitter current is similar
        if (ALMOSTEQUAL(ABS(baseCurrent) + ABS(collectorCurrent), emitterCurrent, .05)) {
            // check current direction and their relative size, so a collector & base can be correctly detected
            if (baseCurrent < -5 && collectorCurrent < -50 && emitterCurrent > 50 && collectorCurrent < baseCurrent) {
                currentComponent.type = ComponentType::BJT_NPN;
                // now we need to check the orientation: some NPN transistors will conduct in both directions (Collector -> Emitter and vice versa)
                // to make sure we have the right collector & emitter pins identified, we need to reverse the voltage on both of the pins
                // and check the beta then
                // the combination that makes for the biggest beta value identifies the right orientation
                double firstBeta = collectorCurrent / baseCurrent;
                // so now we reverse "third" and "first": first pin is considered to be the emitter, third is considered to be the collector
                // set emitter as GND, so that VBE = 0.7V
                ProbeCombination::possibleCombinations[i].first->setVoltage(0);
                // set collector as 500mV, so the transistor can conduct
                ProbeCombination::possibleCombinations[i].third->setVoltage(500);
                // wait for a short time
                sleep_ms(10);
                // check if transistor is actually conducting
                Current secondBaseCurrent = ProbeCombination::possibleCombinations[i].second->readAverageCurrent(10);
                if (secondBaseCurrent < -5) {
                    // use the new currents to calculate the other beta
                    double secondBeta = ProbeCombination::possibleCombinations[i].third->readAverageCurrent(10) / secondBaseCurrent;
                    if (ABS(firstBeta) > ABS(secondBeta)) {
                        // the original orientation resulted in a bigger beta, so it was right the first time
                        currentComponent.data.bjtNpnData.collectorBaseEmitterPins = ProbeCombination::possibleCombinations[i];
                        // turn off the probes
                        ProbeCombination::possibleCombinations[i].first->turnOff();
                        ProbeCombination::possibleCombinations[i].second->turnOff();
                        ProbeCombination::possibleCombinations[i].third->turnOff();
                        return;
                    } else {
                        // the new orientation resulted in a bigger beta, so collector & emitter have to be swapped
                        currentComponent.data.bjtNpnData.collectorBaseEmitterPins = 
                        { ProbeCombination::possibleCombinations[i].third, ProbeCombination::possibleCombinations[i].second, ProbeCombination::possibleCombinations[i].first, ProbeCombination::possibleCombinations[i].thirdPinNumber, ProbeCombination::possibleCombinations[i].secondPinNumber, ProbeCombination::possibleCombinations[i].firstPinNumber };
                        // turn off the probes
                        ProbeCombination::possibleCombinations[i].first->turnOff();
                        ProbeCombination::possibleCombinations[i].second->turnOff();
                        ProbeCombination::possibleCombinations[i].third->turnOff();
                        return;
                    }
                } else {
                    // the original orientation was the only one actually conducting, so this is the right orientation
                    currentComponent.data.bjtNpnData.collectorBaseEmitterPins = ProbeCombination::possibleCombinations[i];
                    // turn off the probes
                    ProbeCombination::possibleCombinations[i].first->turnOff();
                    ProbeCombination::possibleCombinations[i].second->turnOff();
                    ProbeCombination::possibleCombinations[i].third->turnOff();
                    return;
                }
            }
        }
    }
    // check if DUT is a BJT PNP transistor
    for (unsigned char i = 0; i < 6; ++i) {
        // first pin is considered to be the collector, second as base and third as emitter
        // set emitter as 1000mV, so that VBE = -0.7V
        ProbeCombination::possibleCombinations[i].third->setVoltage(1000);
        // set base as 300mV, so that VBE = -0.7V
        ProbeCombination::possibleCombinations[i].second->setVoltage(300);
        // set collector as 500mV, so that VCE >= 0.2V
        ProbeCombination::possibleCombinations[i].first->setVoltage(500);
        // wait for a short time
        sleep_ms(10);
        // check if it is a BJT using current measurements
        Current baseCurrent = ProbeCombination::possibleCombinations[i].second->readAverageCurrent(10);
        Current emitterCurrent = ProbeCombination::possibleCombinations[i].third->readAverageCurrent(10);
        Current collectorCurrent = ProbeCombination::possibleCombinations[i].first->readAverageCurrent(10);
        // check if collector and emitter current is similar
        if (ALMOSTEQUAL(ABS(baseCurrent) + ABS(collectorCurrent), emitterCurrent, .05)) {
            // check current direction
            if (baseCurrent > 5 && collectorCurrent > 50 && emitterCurrent < -50 && collectorCurrent > baseCurrent) {
                currentComponent.type = ComponentType::BJT_PNP;
                // now we need to check the orientation: some NPN transistors will conduct in both directions (Collector -> Emitter and vice versa)
                // to make sure we have the right collector & emitter pins identified, we need to reverse the voltage on both of the pins
                // and check the beta then
                // the combination that makes for the biggest beta value identifies the right orientation
                double firstBeta = collectorCurrent / baseCurrent;
                // so now we reverse "third" and "first": first pin is considered to be the emitter, third is considered to be the collector
                // set emitter as GND, so that VBE = 0.7V
                ProbeCombination::possibleCombinations[i].first->setVoltage(0);
                // set collector as 500mV, so the transistor can conduct
                ProbeCombination::possibleCombinations[i].third->setVoltage(500);
                // wait for a short time
                sleep_ms(10);
                // check if transistor is actually conducting
                Current secondBaseCurrent = ProbeCombination::possibleCombinations[i].second->readAverageCurrent(10);
                if (secondBaseCurrent < -5) {
                    // use the new currents to calculate the other beta
                    double secondBeta = ProbeCombination::possibleCombinations[i].third->readAverageCurrent(10) / secondBaseCurrent;
                    if (ABS(firstBeta) > ABS(secondBeta)) {
                        // the original orientation resulted in a bigger beta, so it was right the first time
                        currentComponent.data.bjtPnpData.collectorBaseEmitterPins = ProbeCombination::possibleCombinations[i];
                        // turn off the probes
                        ProbeCombination::possibleCombinations[i].first->turnOff();
                        ProbeCombination::possibleCombinations[i].second->turnOff();
                        ProbeCombination::possibleCombinations[i].third->turnOff();
                        return;
                    } else {
                        // the new orientation resulted in a bigger beta, so collector & emitter have to be swapped
                        currentComponent.data.bjtPnpData.collectorBaseEmitterPins = 
                        { ProbeCombination::possibleCombinations[i].third, ProbeCombination::possibleCombinations[i].second, ProbeCombination::possibleCombinations[i].first, ProbeCombination::possibleCombinations[i].thirdPinNumber, ProbeCombination::possibleCombinations[i].secondPinNumber, ProbeCombination::possibleCombinations[i].firstPinNumber };
                        // turn off the probes
                        ProbeCombination::possibleCombinations[i].first->turnOff();
                        ProbeCombination::possibleCombinations[i].second->turnOff();
                        ProbeCombination::possibleCombinations[i].third->turnOff();
                        return;
                    }
                } else {
                    // the original orientation was the only one actually conducting, so this is the right orientation
                    currentComponent.data.bjtPnpData.collectorBaseEmitterPins = ProbeCombination::possibleCombinations[i];
                    // turn off the probes
                    ProbeCombination::possibleCombinations[i].first->turnOff();
                    ProbeCombination::possibleCombinations[i].second->turnOff();
                    ProbeCombination::possibleCombinations[i].third->turnOff();
                    return;
                }
            }
        }
    }
    // check if DUT is a diode
    for (unsigned char i = 0; i < 3; ++i) {
        // turn off the third probe
        ProbeCombination::possibleCombinations[i].third->turnOff();
        // set the second probe as GND
        ProbeCombination::possibleCombinations[i].second->setVoltage(0);
        // set the first probe as VCC (750 mV)
        ProbeCombination::possibleCombinations[i].first->setVoltage(750);
        // wait for a short time
        sleep_ms(10);
        // check if a valid voltage drop occured, with a 10% margin of error, and current flows
        MeasureResult results1 = ProbeCombination::possibleCombinations[i].first->doFullMeasure(10);
        MeasureResult results2 = ProbeCombination::possibleCombinations[i].second->doFullMeasure(10);
        if (ALMOSTEQUAL(results1.avgV - results2.avgV, 700, 0.1) && results1.avgA < 100 && results2.avgA > 100) {
            // check if no current flows in reverse bias
            ProbeCombination::possibleCombinations[i].second->setVoltage(750);
            ProbeCombination::possibleCombinations[i].first->setVoltage(0);
            sleep_ms(10);
            results1 = ProbeCombination::possibleCombinations[i].first->doFullMeasure(10);
            results2 = ProbeCombination::possibleCombinations[i].second->doFullMeasure(10);
            if (ABS(results1.avgA) < 100 && ABS(results2.avgA) < 100) {
                currentComponent.type = ComponentType::DIODE;
                currentComponent.data.diodeData.connectedPins = ProbeCombination::possibleCombinations[i];
                return;
            }
        }
    }
    //TODO MOSFET_NMOS, MOSFET_PMOS, MOSFET_JFET
    currentComponent.type = ComponentType::UNKNOWN_DEVICE;
}

void UserInterface::init(int* argc, char *** argv) {
    // start probes
    Probe::init();

    ProbeCombination::possibleCombinations = new ProbeCombination[6] {
        { &Probe::probe[0], &Probe::probe[1], &Probe::probe[2], 1, 2, 3 },
        { &Probe::probe[1], &Probe::probe[2], &Probe::probe[0], 2, 3, 1 },
        { &Probe::probe[0], &Probe::probe[2], &Probe::probe[1], 1, 3, 2 },
        { &Probe::probe[1], &Probe::probe[0], &Probe::probe[2], 2, 1, 3 },
        { &Probe::probe[2], &Probe::probe[1], &Probe::probe[0], 3, 2, 1 },
        { &Probe::probe[2], &Probe::probe[0], &Probe::probe[1], 3, 1, 2 }
    };

    // init GUI
    GtkBuilder* builder;
    GtkWidget* window;
    #ifndef WINDOWS
    XInitThreads();
    #endif
    gtk_init(argc, argv);
    builder = gtk_builder_new_from_file("../ui/landing_page.xml");
    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    gtk_builder_connect_signals(builder, NULL);

    // set top panel pointers
    mainWindow.topPanel.startButton = GTK_BUTTON(gtk_builder_get_object(builder, "start_measuring"));

    for (unsigned char i = 0; i < 3; ++i) {
        mainWindow.topPanel.settings.description[i] = GTK_LABEL(gtk_builder_get_object(builder, MeasureProperties::descriptionNames[i]));
        mainWindow.topPanel.settings.currentValue[i] = GTK_LABEL(gtk_builder_get_object(builder, MeasureProperties::currentValueNames[i]));
        mainWindow.topPanel.settings.incrementValue[i] = GTK_BUTTON(gtk_builder_get_object(builder, MeasureProperties::incrementValueNames[i]));
        mainWindow.topPanel.settings.decrementValue[i] = GTK_BUTTON(gtk_builder_get_object(builder, MeasureProperties::decrementValueNames[i]));
    }

    for (unsigned char i = 0; i < 4; ++i) {
        mainWindow.topPanel.properties.property[i] = GTK_LABEL(gtk_builder_get_object(builder, ComponentProperties::propertyNames[i]));
    }

    mainWindow.topPanel.component.componentName = GTK_LABEL(gtk_builder_get_object(builder, "component_name"));

    for (unsigned char i = 0; i < 3; ++i) {
        mainWindow.topPanel.component.pinout[i] = GTK_LABEL(gtk_builder_get_object(builder, ComponentLayout::pinoutNames[i]));
    }

    mainWindow.topPanel.component.symbol = GTK_IMAGE(gtk_builder_get_object(builder, "component_symbol"));

    mainWindow.topPanel.help = GTK_BUTTON(gtk_builder_get_object(builder, "help"));

    // set bottom panel pointers
    mainWindow.bottomPanel.toggle1 = GTK_BUTTON(gtk_builder_get_object(builder, "toggle_1"));
    mainWindow.bottomPanel.toggle2 = GTK_BUTTON(gtk_builder_get_object(builder, "toggle_2"));
    mainWindow.bottomPanel.toggle3 = GTK_BUTTON(gtk_builder_get_object(builder, "toggle_3"));

    mainWindow.bottomPanel.graphWindow.self = GTK_WIDGET(gtk_builder_get_object(builder, "graph"));

    mainWindow.bottomPanel.graphWindow.unit0 = GTK_LABEL(gtk_builder_get_object(builder, "unit_0"));
    mainWindow.bottomPanel.graphWindow.unit1 = GTK_LABEL(gtk_builder_get_object(builder, "unit_1"));
    mainWindow.bottomPanel.graphWindow.unit2 = GTK_LABEL(gtk_builder_get_object(builder, "unit_2"));
    mainWindow.bottomPanel.graphWindow.unit3 = GTK_LABEL(gtk_builder_get_object(builder, "unit_3"));

    mainWindow.bottomPanel.graphWindow.graphTitle = GTK_LABEL(gtk_builder_get_object(builder, "graph_title"));

    for (unsigned char i = 0; i < 9; ++i) {
        mainWindow.bottomPanel.graphWindow.yLabelsLeft[i] = GTK_LABEL(gtk_builder_get_object(builder, GraphWindow::yLabelsLeftNames[i]));
        mainWindow.bottomPanel.graphWindow.yLabelsRight[i] = GTK_LABEL(gtk_builder_get_object(builder, GraphWindow::yLabelsRightNames[i]));
    }

    for (unsigned char i = 0; i < 21; ++i) {
        mainWindow.bottomPanel.graphWindow.xLabels[i] = GTK_LABEL(gtk_builder_get_object(builder, GraphWindow::xLabelsNames[i]));
    }

    calibrationDialog.self = GTK_DIALOG(gtk_builder_get_object(builder, "calibration_dialog"));
    calibrationDialog.progress_bar = GTK_PROGRESS_BAR(gtk_builder_get_object(builder, "calibrate_progress"));
    calibrationDialog.dialog_close_button = GTK_BUTTON(gtk_builder_get_object(builder, "close_dialog"));
    calibrationDialog.start_button = GTK_BUTTON(gtk_builder_get_object(builder, "calibrate"));

    #ifdef USING_RPI
    // make it so the close button cant be pressed when using the RPI without calibration first
    gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.dialog_close_button, FALSE);
    #endif

    measurementThread = std::thread([]() {
        while (!calibrated && programAlive) {
            sleep_ms(100);
        }
        while (programAlive) {
            if (determenRequested) {
                determineType();
                mainWindow.topPanel.component.update();
                switch (currentComponent.type) {
                    case RESISTOR: {
                        currentComponent.data.resistorData.measure();
                        mainWindow.topPanel.component.update();
                        break;
                    }
                    case CAPACITOR: {
                        currentComponent.data.capacitorData.measure();
                        mainWindow.topPanel.component.update();
                        break;
                    }
                    case DIODE: {
                        currentComponent.data.diodeData.measure();
                        mainWindow.topPanel.component.update();
                        break;
                    }
                    case BJT_NPN: {
                        currentComponent.data.bjtNpnData.measure();
                        mainWindow.topPanel.component.update();
                        break;
                    }
                    case BJT_PNP: {
                        
                        break;
                    }
                    case MOSFET_NMOS: {
                        // TODO
                        
                        break;
                    }
                    case MOSFET_PMOS: {
                        // TODO
                        
                        break;
                    }
                    case MOSFET_JFET: {
                        // TODO
                        
                        break;
                    }
                    case UNKNOWN_DEVICE:
                    default: {
                        break;
                    }
                }
                mainWindow.topPanel.properties.update();
                determenRequested = false;
            }
            if (measurementRequested) {
                switch (currentComponent.type) {
                    case BJT_NPN: {
                        currentComponent.data.bjtNpnData.graphIbIc();
                        mainWindow.bottomPanel.graphWindow.updateGraph();
                        break;
                    }
                    case BJT_PNP: {
                        
                        break;
                    }
                    case MOSFET_NMOS: {
                        // TODO
                        
                        break;
                    }
                    case MOSFET_PMOS: {
                        // TODO
                        
                        break;
                    }
                    case MOSFET_JFET: {
                        // TODO
                        
                        break;
                    }
                    case UNKNOWN_DEVICE:
                    default: {
                        break;
                    }
                }
                measurementRequested = false;
            }
            sleep_ms(100);
        }
    });

    g_object_unref(builder);
    // enable main screen
    gtk_widget_show(window);
    // show calibration dialog
    gtk_widget_show((GtkWidget*) calibrationDialog.self);
    // make unused labels invisible
    gtk_widget_set_opacity((GtkWidget*) mainWindow.bottomPanel.graphWindow.unit0, 0.0);
    gtk_widget_set_opacity((GtkWidget*) mainWindow.bottomPanel.graphWindow.unit3, 0.0);
    // make graph invisible
    mainWindow.bottomPanel.graphWindow.makeCompletelyInvisible();
    mainWindow.topPanel.properties.setInvisible();
    currentComponent.type = UNKNOWN_DEVICE;
    mainWindow.topPanel.component.update();
    // start gtk functionality
    gtk_main();
}

unsigned int segment;
double segmentProgress;

void updateProgress() {
    g_idle_add(G_SOURCE_FUNC(+[](){ gtk_progress_bar_set_fraction(calibrationDialog.progress_bar, 0.33 * segment + segmentProgress / 3.0); return FALSE; }), NULL);
}

void calibrate() {
    calibrationThread = std::thread([]() {
        gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.dialog_close_button, FALSE);
        gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.start_button, FALSE);
        // determine probe offset
        for (unsigned char i = 0; i < 3; ++i) {
            Probe::probe[i].setVoltage(0);
            sleep_ms(10);
            UVoltage error = Probe::probe[i].readVoltage();
            if (error > 0) {
                Probe::probe[i].setOffset(error - 1);
            }
        }
        // set shunt
        // TODO maybe make it more accurate by using known resistors
        Probe::probe[0].setShunt(3.88);
        Probe::probe[1].setShunt(3.88);
        Probe::probe[2].setShunt(3.88);
        // set GUI to indicate probe 1 is getting calibrated
        g_idle_add(G_SOURCE_FUNC(+[](){gtk_progress_bar_set_text(calibrationDialog.progress_bar, "Kalibreren... Probe 1/3"); return FALSE;}), NULL);
        segment = 0;
        Probe::probe[0].calibrate(updateProgress, &segmentProgress);
        g_idle_add(G_SOURCE_FUNC(+[](){gtk_progress_bar_set_text(calibrationDialog.progress_bar, "Kalibreren... Probe 2/3"); return FALSE;}), NULL);
        ++segment;
        Probe::probe[1].calibrate(updateProgress, &segmentProgress);
        g_idle_add(G_SOURCE_FUNC(+[](){gtk_progress_bar_set_text(calibrationDialog.progress_bar, "Kalibreren... Probe 3/3"); return FALSE;}), NULL);
        ++segment;
        Probe::probe[2].calibrate(updateProgress, &segmentProgress);
        g_idle_add(G_SOURCE_FUNC(+[](){gtk_progress_bar_set_text(calibrationDialog.progress_bar, "Klaar!"); gtk_progress_bar_set_fraction(calibrationDialog.progress_bar, 1.0); gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.dialog_close_button, TRUE); return FALSE;}), NULL);
        calibrated = true;
    });
}

void destroy() {
    programAlive = false;
    calibrationThread.join();
    measurementThread.join();
    gtk_main_quit();
    delete[] ProbeCombination::possibleCombinations;
    Probe::destroy();
}
