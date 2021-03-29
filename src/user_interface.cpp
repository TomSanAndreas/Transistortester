#include "user_interface.hpp"
#include <thread>

#ifdef USING_RPI
#include <X11/Xlib.h>
#endif

// determine if both values are at least almost equal, only 1% different max
#define ALMOSTEQUAL(value1, value2, percentage) ABS(ABS(value1) - ABS(value2)) < percentage * ABS(value1)

void calibrate();
void destroy();
void determineType();

struct ProbeCombination {
    Probe* first,* second,* third;
    static ProbeCombination* possibleCombinations;
};

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
};

struct CapacitorData {
    double capacitance;
    ProbeCombination connectedPins;
};

struct DiodeData {
    double voltageDrop;
    ProbeCombination connectedPins;
};

struct BjtNpnData {
    double beta;
    unsigned int saturationVoltageCollectorEmitter;
    unsigned int saturationVoltageBaseEmitter;
    ProbeCombination baseEmitterPins;
    ProbeCombination collectorEmitterPins;
    ProbeCombination collectorBasePins;
};

struct BjtPnpData {
    double beta;
    unsigned int saturationVoltageCollectorEmitter;
    unsigned int saturationVoltageBaseEmitter;
    ProbeCombination baseEmitterPins;
    ProbeCombination collectorEmitterPins;
    ProbeCombination collectorBasePins;
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

struct Point {
    float x, y;
};

struct Graph {
    Point data[50];
    static int maxX, maxY, minX, minY;
};

int Graph::maxX;
int Graph::maxY;
int Graph::minX;
int Graph::minY;

struct CalibrationDialog {
    GtkDialog* self;
    GtkButton* dialoge_close_button,* start_button;
    GtkProgressBar* progress_bar;
};

struct GraphWindow {
    GtkWidget* self;
    GtkLabel* yLabelsLeft[9],* yLabelsRight[9];
    static const char* yLabelsLeftNames[],* yLabelsRightNames[];
    GtkLabel* xLabels[21];
    static const char* xLabelsNames[];
    GtkLabel* unit0,* unit1,* unit2,* unit3;
    GtkLabel* graphTitle;
};

const char* GraphWindow::yLabelsLeftNames[] = {
    "y_0_0", "y_0_1", "y_0_2", "y_0_3", "y_0_4", "y_0_5", "y_0_6", "y_0_7", "y_0_8"
};

const char* GraphWindow::yLabelsRightNames[] = {
    "y_1_0", "y_1_1", "y_1_2", "y_1_3", "y_1_4", "y_1_5", "y_1_6", "y_1_7", "y_1_8"
};

const char* GraphWindow::xLabelsNames[] = {
    "x_0", "x_1", "x_2", "x_3", "x_4", "x_5", "x_6", "x_7", "x_8", "x_9", "x_10", "x_11", "x_12", "x_13", "x_14", "x_15", "x_16", "x_17", "x_18", "x_19", "x_20"
};

struct BottomPanel {
    GtkButton* toggle1,* toggle2,* toggle3;
    GraphWindow graphWindow;
};

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
    GtkLabel* property[4];
    static const char* propertyNames[];
};

const char* ComponentProperties::propertyNames[] = {
    "property_0", "property_1", "property_2", "property_3"
};

struct ComponentLayout {
    GtkLabel* pinout[3];
    static const char* pinoutNames[];
    GtkImage* symbol;
    GtkLabel* componentName;
    static const char* possibleComponentNames[];
};

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


Graph graphs[3];
GdkRGBA colors[3] { {.75, 1, .75, 1}, {1, .75, .75, 1}, {1, .75, .75, 1} };
CalibrationDialog calibrationDialog;

std::thread calibrationThread;

extern "C" {
    #ifdef WINDOWS
    G_MODULE_EXPORT void destroy_signal(GtkWidget* w, gpointer user_data) {
        destroy();
    }
    G_MODULE_EXPORT gboolean draw_signal(GtkWidget* widget, cairo_t* cr, gpointer data) {
        // afmetingen grafiek in pixels bepalen
        unsigned int width = gtk_widget_get_allocated_width(widget);
        unsigned int height = gtk_widget_get_allocated_height(widget);
        // schalen berekenen
        double scaleX, scaleY;
        scaleX = ((double) width) / (Graph::maxX - Graph::minX);
        scaleY = ((double) height) / (Graph::maxY - Graph::minY);
        // raster tekenen: 21 verticale lijnen en 9 horizontale lijnen op gelijke afstand
        cairo_set_source_rgba(cr, 0, 0, 0, .1);
        int nPixelsVertical = height / 9;
        for (unsigned int i = nPixelsVertical >> 1; i < 11 * nPixelsVertical; i += nPixelsVertical) {
            cairo_line_to(cr, 0.01 * width, i);
            cairo_line_to(cr, 0.99 * width, i);
            cairo_stroke(cr);
        }
        int nPixelsHorizontal = width / 21;
        for (unsigned int i = nPixelsHorizontal >> 1; i < 23 * nPixelsHorizontal; i += nPixelsHorizontal) {
            cairo_line_to(cr, i, 0.01 * height);
            cairo_line_to(cr, i, 0.99 * height);
            cairo_stroke(cr);
        }
        // plotten grafieken
        for (unsigned char i = 0; i < 3; ++i) {
            for (unsigned int j = 0; j < 50; ++j) {
                cairo_line_to(cr, graphs[i].data[j].x * scaleX, height - graphs[i].data[j].y * scaleY);
            }
            gdk_cairo_set_source_rgba(cr, &colors[i]);
            cairo_stroke(cr);
        }
        return false;
    }
    G_MODULE_EXPORT void start_calibration(GtkWidget* widget, gpointer user_data) {
        calibrate();
    }
    G_MODULE_EXPORT void close_dialog(GtkWidget* widget, gpointer user_data) {
        gtk_widget_destroy((GtkWidget*) calibrationDialog.self);
    }
    G_MODULE_EXPORT void determine_type(GtkWidget* widget, gpointer user_data) {
        determineType();
        gtk_label_set_text(mainWindow.topPanel.component.componentName, ComponentProperties::propertyNames[currentComponent.type]);
    }
    #elif USING_RPI
    void destroy_signal(GtkWidget* w, gpointer user_data) {
        destroy();
    }
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
        int nPixelsVertical = height / 9;
        for (unsigned int i = nPixelsVertical >> 1; i < 11 * nPixelsVertical; i += nPixelsVertical) {
            cairo_line_to(cr, 0.01 * width, i);
            cairo_line_to(cr, 0.99 * width, i);
            cairo_stroke(cr);
        }
        int nPixelsHorizontal = width / 21;
        for (unsigned int i = nPixelsHorizontal >> 1; i < 23 * nPixelsHorizontal; i += nPixelsHorizontal) {
            cairo_line_to(cr, i, 0.01 * height);
            cairo_line_to(cr, i, 0.99 * height);
            cairo_stroke(cr);
        }
        // plotten grafieken
        for (unsigned char i = 0; i < 3; ++i) {
            for (unsigned int j = 0; j < 50; ++j) {
                cairo_line_to(cr, graphs[i].data[j].x * scaleX, height - graphs[i].data[j].y * scaleY);
            }
            gdk_cairo_set_source_rgba(cr, &colors[i]);
            cairo_stroke(cr);
        }
        return false;
    }
    void start_calibration(GtkWidget* widget, gpointer user_data) {
        calibrate();
    }
    void close_dialog(GtkWidget* widget, gpointer user_data) {
        gtk_widget_destroy((GtkWidget*) calibrationDialog.self);
    }
    void determine_type(GtkWidget* widget, gpointer user_data) {
        determineType();
    }
    #endif
}

void measure() {
    // MeasureResult data1, data2;
    // Probe::probe[start.second].setVoltage(0);
    // for (unsigned int i = 1; i < 51; ++i) {
    //     Probe::probe[start.first].setVoltage(i * 100);
    //     sleep_ms(1);
    //     data1 = Probe::probe[start.first].doFullMeasure(5);
    //     data2 = Probe::probe[start.second].doFullMeasure(5);
    //     for (unsigned char j = 0; j < 3; ++j) {
    //         graphs[j].data[i - 1].x = data1.usedVoltage;
    //     }
    //     graphs[0].data[i - 1].y = (data1.avgV - data2.avgV) / data1.avgA;
    //     graphs[1].data[i - 1].y = (data1.minV - data2.minV) / data1.maxA;
    //     graphs[2].data[i - 1].y = (data1.maxV - data2.maxV) / data1.minA;

    //     graphs[0].data[i - 1].x = i;
    //     graphs[1].data[i - 1].x = i;
    //     graphs[2].data[i - 1].x = i;
    //     graphs[0].data[i - 1].y = 5*i;
    //     graphs[1].data[i - 1].y = 1 / i;
    //     graphs[2].data[i - 1].y = 2 * i *i;
    // }
    // // maxima & minima instellen, eerst oude maxima & minima "resetten"
    // Graph::minX = graphs[0].data[0].x;
    // Graph::maxX = graphs[0].data[0].x;
    // Graph::minY = graphs[0].data[0].y;
    // Graph::maxY = graphs[0].data[0].y;
    // for (unsigned char i = 0; i < 3; ++i) {
    //     for (unsigned int j = 0; j < 50; ++j) {
    //         if (Graph::minX > graphs[i].data[j].x) {
    //             Graph::minX = graphs[i].data[j].x;
    //         }
    //         if (Graph::maxX < graphs[i].data[j].x) {
    //             Graph::maxX = graphs[i].data[j].x;
    //         }
    //         if (Graph::minY > graphs[i].data[j].y) {
    //             Graph::minY = graphs[i].data[j].y;
    //         }
    //         if (Graph::maxY < graphs[i].data[j].y) {
    //             Graph::maxY = graphs[i].data[j].y;
    //         }
    //     }
    // }
    // // TODO: forceer graph draw
    // // updaten graph labels
    // char buffer[10];
    // for (unsigned char i = 0; i < 21; ++i) {
    //     sprintf(buffer, "%4d%*s", (i + 1) * (Graph::maxX - Graph::minX) / 21, 2, " ");
    //     gtk_label_set_text(mainWindow.bottomPanel.graphWindow.xLabels[i], buffer);
    // }
    // for (unsigned char i = 0; i < 9; ++i) {
    //     sprintf(buffer, "%8d", (i + 1) * (Graph::maxY - Graph::minY) / 9);
    //     gtk_label_set_text(mainWindow.bottomPanel.graphWindow.yLabelsLeft[i], buffer);
    //     // rechterlabels worden momenteel niet gebruikt
    //     // gtk_label_set_text(mainWindow.bottomPanel.graphWindow.yLabelsRight[i], buffer);
    //     gtk_widget_set_opacity((GtkWidget*) mainWindow.bottomPanel.graphWindow.yLabelsRight[i], 0.0);
    // }
    // // updaten graph units
    // gtk_label_set_text(mainWindow.bottomPanel.graphWindow.unit0, "");
    // gtk_label_set_text(mainWindow.bottomPanel.graphWindow.unit1, "mV");
    // gtk_label_set_text(mainWindow.bottomPanel.graphWindow.unit2, "mA");
    // // rechterlabel wordt momenteel niet gebruikt
    // // gtk_label_set_text(mainWindow.bottomPanel.graphWindow.unit3, "mA");
    // gtk_widget_set_opacity((GtkWidget*) mainWindow.bottomPanel.graphWindow.unit3, 0.0);
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
        if ((results1.avgA < -25 && results2.avgA > 25) || (results1.avgA > 25 && results2.avgA < -25) && ALMOSTEQUAL(results2.avgA, results1.avgA, 0.01)) {
            currentComponent.type = ComponentType::RESISTOR;
            currentComponent.data.resistorData.connectedPins = ProbeCombination::possibleCombinations[i];
            currentComponent.data.resistorData.resistance = (results1.avgV - results2.avgV) / (results2.avgA);
            return;
        }
    }
    // check if DUT is a capacitor
    //TODO
    // check if DUT is a diode
    for (unsigned char i = 0; i < 6; ++i) {
        // turn off the third probe
        ProbeCombination::possibleCombinations[i].third->turnOff();
        // set the second probe as GND
        ProbeCombination::possibleCombinations[i].second->setVoltage(0);
        // set the first probe as VCC (750 mV)
        ProbeCombination::possibleCombinations[i].first->setVoltage(750);
        // wait for a short time
        sleep_ms(10);
        // check if a valid voltage drop occured, with a 10% margin of error
        MeasureResult results1 = ProbeCombination::possibleCombinations[i].first->doFullMeasure(10);
        MeasureResult results2 = ProbeCombination::possibleCombinations[i].second->doFullMeasure(10);
        if (ALMOSTEQUAL(results1.avgV - results2.avgV, 700, 0.1)) {
            currentComponent.type = ComponentType::DIODE;
            currentComponent.data.diodeData.connectedPins = ProbeCombination::possibleCombinations[i];
            currentComponent.data.diodeData.voltageDrop = results1.avgV - results2.avgV;
            return;
        }
    }
    // check if DUT is a BJT transistor (NPN or PNP)
    for (unsigned char i = 0; i < 3; ++i) {
        // first pin is considered to be the collector, second as base and third as emitter
        // set emitter as GND
        ProbeCombination::possibleCombinations[i].third->setVoltage(0);
        // set base as 700mV
        ProbeCombination::possibleCombinations[i].second->setVoltage(700);
        // set collector as 1000mV
        ProbeCombination::possibleCombinations[i].first->setVoltage(1000);
        // wait for a short time
        sleep_ms(10);
        // check if it is a BJT using the measurements
        MeasureResult results1 = ProbeCombination::possibleCombinations[i].first->doFullMeasure(10);
        MeasureResult results2 = ProbeCombination::possibleCombinations[i].second->doFullMeasure(10);
        MeasureResult results3 = ProbeCombination::possibleCombinations[i].third->doFullMeasure(10);
        // check if collector and emitter current is similar
        if (ALMOSTEQUAL(results1.avgA, results3.avgA, .01)) {
            // check currents to see if it is a PNP or NPN
            // NPN
            if (results2.avgA > 0 && results1.avgA < 0 && results3.avgA > 0) {
                currentComponent.type = ComponentType::BJT_NPN;
                currentComponent.data.bjtNpnData.baseEmitterPins = {
                    ProbeCombination::possibleCombinations[i].second, ProbeCombination::possibleCombinations[i].third, ProbeCombination::possibleCombinations[i].first
                };
                currentComponent.data.bjtNpnData.collectorEmitterPins = {
                    ProbeCombination::possibleCombinations[i].first, ProbeCombination::possibleCombinations[i].third, ProbeCombination::possibleCombinations[i].second
                };
                currentComponent.data.bjtNpnData.collectorBasePins = {
                    ProbeCombination::possibleCombinations[i].first, ProbeCombination::possibleCombinations[i].second, ProbeCombination::possibleCombinations[i].third
                };
                return;
            }
            // PNP
            else if (results2.avgA < 0 && results1.avgA > 0 && results3.avgA < 0) {
                currentComponent.type = ComponentType::BJT_PNP;
                currentComponent.data.bjtNpnData.baseEmitterPins = {
                    ProbeCombination::possibleCombinations[i].second, ProbeCombination::possibleCombinations[i].third, ProbeCombination::possibleCombinations[i].first
                };
                currentComponent.data.bjtNpnData.collectorEmitterPins = {
                    ProbeCombination::possibleCombinations[i].first, ProbeCombination::possibleCombinations[i].third, ProbeCombination::possibleCombinations[i].second
                };
                currentComponent.data.bjtNpnData.collectorBasePins = {
                    ProbeCombination::possibleCombinations[i].first, ProbeCombination::possibleCombinations[i].second, ProbeCombination::possibleCombinations[i].third
                };
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
        { &Probe::probe[0], &Probe::probe[1], &Probe::probe[2] }, // 0, 1, 2
        { &Probe::probe[1], &Probe::probe[2], &Probe::probe[0] }, // 1, 2, 0
        { &Probe::probe[0], &Probe::probe[2], &Probe::probe[1] }, // 0, 2, 1
        { &Probe::probe[1], &Probe::probe[0], &Probe::probe[2] }, // 1, 0, 2
        { &Probe::probe[2], &Probe::probe[1], &Probe::probe[0] }, // 2, 1, 0
        { &Probe::probe[2], &Probe::probe[0], &Probe::probe[1] }, // 2, 0, 1
    };

    // init GUI
    GtkBuilder* builder;
    GtkWidget* window,* graph;
    #ifdef USING_RPI
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
    calibrationDialog.dialoge_close_button = GTK_BUTTON(gtk_builder_get_object(builder, "close_dialog"));
    calibrationDialog.start_button = GTK_BUTTON(gtk_builder_get_object(builder, "calibrate"));
    g_object_unref(builder);
    gtk_widget_show(window);
    gtk_widget_show((GtkWidget*) calibrationDialog.self);
    gtk_main();
}

unsigned int ID;

void setProgress(double PROGRESS) {
    gtk_progress_bar_set_fraction(calibrationDialog.progress_bar, 0.33 * ID + (((double) PROGRESS) / 3));
}

void calibrate() {
    calibrationThread = std::thread([]() {
        gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.dialoge_close_button, FALSE);
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
        ID = 0;
        gtk_progress_bar_set_text(calibrationDialog.progress_bar, "Kalibreren... Probe 1/3");
        Probe::probe[0].calibrate(setProgress);
        gtk_progress_bar_set_text(calibrationDialog.progress_bar, "Kalibreren... Probe 2/3");
        ++ID;
        Probe::probe[1].calibrate(setProgress);
        gtk_progress_bar_set_text(calibrationDialog.progress_bar, "Kalibreren... Probe 3/3");
        ++ID;
        Probe::probe[2].calibrate(setProgress);
        gtk_progress_bar_set_text(calibrationDialog.progress_bar, "Klaar!");
        gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.dialoge_close_button, TRUE);
    });
}

void destroy() {
    calibrationThread.join();
    gtk_main_quit();
    delete[] ProbeCombination::possibleCombinations;
    Probe::destroy();
}