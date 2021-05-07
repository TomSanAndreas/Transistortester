#include "user_interface.hpp"
#include <thread>
#include <ctime>
#include <fstream>

#include "bjtnpn.hpp"
#include "bjtpnp.hpp"
#include "diode.hpp"
#include "resistor.hpp"

#ifndef WINDOWS
#include <X11/Xlib.h>
#endif

#define UPPER_SAMPLE_LIMIT 100
#define LOWER_SAMPLE_LIMIT 1

#define UPPER_POINT_LIMIT 200
#define LOWER_POINT_LIMIT 25

#define CSV_DELIMITER ';'

void destroy();
void determineType();
// flags used in the seperate thread
bool determenRequested = false, measurementRequested = false, programAlive = false, calibrationRequested = false, saveRequested = false;

struct CalibrationDialog {
    GtkDialog* self;
    GtkButton* dialog_close_button,* start_button;
    GtkProgressBar* progress_bar;
};

struct HelpDialog {
    GtkDialog* self;
    GtkButton* close;
    GtkLabel* title;
};

struct MeasureProperties {
    static GtkLabel* description[3];
    static const char* descriptionNames[];
    static GtkLabel* currentValue[2];
    static unsigned int currentValueInt[2];
    static const char* currentValueNames[];
    static GtkButton* decrementValue[2];
    static const char* decrementValueNames[];
    static GtkButton* incrementValue[2];
    static const char* incrementValueNames[];
    static GtkCheckButton* sampleVoltageButton;
    static const char* sampleVoltageButtonLabel;
    static bool shouldSampleVoltage;
};

GtkLabel* MeasureProperties::description[3];
GtkLabel* MeasureProperties::currentValue[2];
unsigned int MeasureProperties::currentValueInt[2];
GtkButton* MeasureProperties::incrementValue[2];
GtkButton* MeasureProperties::decrementValue[2];
GtkCheckButton* MeasureProperties::sampleVoltageButton;

const char* MeasureProperties::descriptionNames[] = {
    "samples_label", "punten_label", "empty_label"
};

const char* MeasureProperties::currentValueNames[] = {
    "samples_counter", "punten_counter"
};

const char* MeasureProperties::decrementValueNames[] = {
    "samples_decrement", "punten_decrement"
};

const char* MeasureProperties::incrementValueNames[] = {
    "samples_increment", "punten_increment"
};

bool MeasureProperties::shouldSampleVoltage = false;

const char* MeasureProperties::sampleVoltageButtonLabel = "sample_voltage";

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
    static bool isStable;
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
                if (MeasureProperties::shouldSampleVoltage && GraphContext::data[Graph::graphType].canMeasureVoltage) {
                    gtk_widget_set_opacity((GtkWidget*) yLabelsRight[i], 1.0);
                } else {
                    gtk_widget_set_opacity((GtkWidget*) yLabelsRight[i], 0.0);
                }
            }
            // zet de eenheden op het einde van de X- en Y-as zichtbaar
            gtk_widget_set_opacity((GtkWidget*) unit1, 1.0);
            gtk_widget_set_opacity((GtkWidget*) unit2, 1.0);
            if (MeasureProperties::shouldSampleVoltage && GraphContext::data[Graph::graphType].canMeasureVoltage) {
                gtk_widget_set_opacity((GtkWidget*) unit3, 1.0);
            } else {
                gtk_widget_set_opacity((GtkWidget*) unit3, 0.0);
            }
            // stel de eenheden op het einde van de X- en Y-as in
            gtk_label_set_text(unit1, GraphContext::data[Graph::graphType].xUnit);
            gtk_label_set_text(unit2, GraphContext::data[Graph::graphType].yUnit1);
            gtk_label_set_text(unit3, GraphContext::data[Graph::graphType].yUnit2);
            // zet de titel zichtbaar
            gtk_widget_set_opacity((GtkWidget*) graphTitle, 1.0);
            // stel de titel in
            gtk_label_set_text(graphTitle, GraphContext::data[Graph::graphType].graphTitle);
            // grafiek zichtbaar maken
            gtk_widget_set_opacity(self, 1.0);
            // aangeven dat de data stabiel is
            isStable = true;
            // grafiek opnieuw tekenen
            gtk_widget_queue_draw(self);
            // labels zetten
            char buffer[10];
            for (unsigned int i = 0; i < 21; ++i) {
                sprintf(buffer, "%.1f", ((float) (i + 1) * (Graph::maxX - Graph::minX) / 21 + Graph::minX) / GraphContext::data[Graph::graphType].scaleFactorX);
                gtk_label_set_text(xLabels[i], buffer);
            }
            for (unsigned int i = 0; i < 9; ++i) {
                sprintf(buffer, "%.1f", ((i + 1) * ((float) (Graph::maxYCurrent - Graph::minYCurrent) / 9) + Graph::minYCurrent) / GraphContext::data[Graph::graphType].scaleFactorY1);
                gtk_label_set_text(yLabelsLeft[i], buffer);
                if (MeasureProperties::shouldSampleVoltage && GraphContext::data[Graph::graphType].canMeasureVoltage) {
                    sprintf(buffer, "%.1f", ((i + 1) * ((float) (Graph::maxYVoltage - Graph::minYVoltage) / 9) + Graph::minYVoltage) / GraphContext::data[Graph::graphType].scaleFactorY2);
                    gtk_label_set_text(yLabelsRight[i], buffer);
                }
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

bool GraphWindow::isStable = false;

// de buttons zijn static, zodanig dat de lambda "this" niet in de capture-list moet hebben
struct BottomPanel {
    static GtkButton* toggle1,* toggle2,* toggle3;
    GraphWindow graphWindow;
    static void disableButtons() {
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
    static void updateButtons() {
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

struct ComponentProperties {
    static GtkLabel* property[4];
    static const char* propertyNames[];
    static void update() {
        // main thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            if (Component::type == UNKNOWN_DEVICE) {
                for (unsigned char i = 0; i < 4; ++i) {
                    gtk_label_set_text(property[i], "\0");
                }
                return FALSE;
            }
            char buffer[60];
            // i = DESCRIPTION_LINE_1 -> DESCRIPTION_LINE_4
            for (unsigned char i = DESCRIPTION_LINE_1; i <= DESCRIPTION_LINE_4; ++i) {
                Component::currentComponent->getPropertyText((PropertyType) i, buffer);
                gtk_label_set_text(property[(unsigned int) i - DESCRIPTION_LINE_1], buffer);
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
            if (Component::type == UNKNOWN_DEVICE) {
                gtk_label_set_text(componentName, "Ongekend apparaat");
                gtk_image_set_from_file(symbol, "../ui/unknown.png");
                return FALSE;
            }
            char buffer[35];
            Component::currentComponent->getPropertyText(COMPONENT_NAME, buffer);
            gtk_label_set_text(componentName, buffer);
            Component::currentComponent->getPropertyText(SYMBOL_NAME, buffer);
            gtk_image_set_from_file(symbol, buffer);
            for (unsigned char i = PINOUT_1; i <= PINOUT_3; ++i) {
                Component::currentComponent->getPropertyText((PropertyType) i, buffer);
                gtk_label_set_text(pinout[i - PINOUT_1], buffer);
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

struct TopPanel {
    static GtkButton* startButton;
    MeasureProperties settings;
    ComponentProperties properties;
    ComponentLayout component;
    static GtkButton* help;
    static GtkButton* save;
    static void disableButtons() {
        // main thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            gtk_widget_set_sensitive((GtkWidget*) startButton, FALSE);
            gtk_widget_set_sensitive((GtkWidget*) help, FALSE);
            gtk_widget_set_sensitive((GtkWidget*) save, FALSE);
            return FALSE;
        }), NULL);
    }
    static void disableSaveKey() {
        // main thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            gtk_widget_set_sensitive((GtkWidget*) save, FALSE);
            return FALSE;
        }), NULL);
    }
    static void enableSaveKey() {
        // main thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            gtk_widget_set_sensitive((GtkWidget*) save, TRUE);
            return FALSE;
        }), NULL);
    }
    static void enableButtons() {
        // main thread
        g_idle_add(G_SOURCE_FUNC(+[]() {
            gtk_widget_set_sensitive((GtkWidget*) startButton, TRUE);
            gtk_widget_set_sensitive((GtkWidget*) help, TRUE);
            gtk_widget_set_sensitive((GtkWidget*) save, TRUE);
            return FALSE;
        }), NULL);
    }
};

GtkButton* TopPanel::startButton,* TopPanel::help,* TopPanel::save;

struct MainWindow {
    TopPanel topPanel;
    BottomPanel bottomPanel;
    static void disableAllButtons() {
        TopPanel::disableButtons();
        BottomPanel::disableButtons();
    }
} mainWindow;

GdkColor colorsC {.red = 43690, .green = 65535, .blue = 43690};
GdkColor colorsV {.red = 43690, .green = 43690, .blue = 65535};
CalibrationDialog calibrationDialog;
HelpDialog helpDialog;

unsigned int segment;
double segmentProgress;

void updateProgress() {
    g_idle_add(G_SOURCE_FUNC(+[](){ gtk_progress_bar_set_fraction(calibrationDialog.progress_bar, 0.33 * segment + segmentProgress / 3.0); return FALSE; }), NULL);
}

std::thread backgroundThread;

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
        // indien er geen data is opgeslagen, of de data als onstabiel is aangeduid, mag er direct gestopt worden met tekenen
        if (Graph::graphCurrent[0].data == nullptr || !GraphWindow::isStable) {
            return false;
        }
        // afmetingen grafiek in pixels bepalen
        unsigned int width = gtk_widget_get_allocated_width(widget);
        unsigned int height = gtk_widget_get_allocated_height(widget);
        // schalen berekenen
        double scaleX, scaleY1, scaleY2;
        scaleX = ((double) width) / (Graph::maxX - Graph::minX);
        scaleY1 = ((double) height) / (Graph::maxYCurrent - Graph::minYCurrent);
        scaleY2 = ((double) height) / (Graph::maxYVoltage - Graph::minYVoltage);
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
        // plotten grafieken, eerst gemiddelde waardes in 1 lijn
        for (unsigned int j = 0; j < Graph::nPoints; ++j) {
            cairo_line_to(cr, (Graph::graphCurrent[0].data[j].x - Graph::minX) * scaleX, height - (Graph::graphCurrent[0].data[j].y - Graph::minYCurrent) * scaleY1);
        }
        gdk_cairo_set_source_color(cr, &colorsC);
        cairo_stroke(cr);
        if (MeasureProperties::shouldSampleVoltage && GraphContext::data[Graph::graphType].canMeasureVoltage && Graph::graphVoltage[0].data != nullptr) {
            for (unsigned int j = 0; j < Graph::nPoints; ++j) {
                cairo_line_to(cr, (Graph::graphVoltage[0].data[j].x - Graph::minX) * scaleX, height - (Graph::graphVoltage[0].data[j].y - Graph::minYVoltage) * scaleY2);
            }
            gdk_cairo_set_source_color(cr, &colorsV);
            cairo_stroke(cr);
        }
        // getransformeerde coordinaten tijdelijk bijhouden van elk punt
        unsigned int x, y;
        // vervolg plotten, extrema via punten
        for (unsigned int i = 1; i < 3; ++i) {
            for (unsigned int j = 0; j < Graph::nPoints; ++j) {
                // coordinaten transformeren punt
                x = (Graph::graphCurrent[i].data[j].x - Graph::minX) * scaleX;
                y = height - (Graph::graphCurrent[i].data[j].y - Graph::minYCurrent) * scaleY1;
                // diameter punt
                cairo_set_line_width(cr, 5);
                // vorm instellen
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                // punt plaatsen
                cairo_move_to(cr, x, y); cairo_line_to(cr, x, y);
                // kleur plaatsen
                gdk_cairo_set_source_color(cr, &colorsC);
                cairo_stroke(cr);
            }
            // gelijkaardige stappen doorlopen voor spanning, indien van toepassing
            if (MeasureProperties::shouldSampleVoltage && GraphContext::data[Graph::graphType].canMeasureVoltage && Graph::graphVoltage[i].data != nullptr) {
                unsigned int yRef;
                for (unsigned int j = 0; j < Graph::nPoints; ++j) {
                    x = (Graph::graphVoltage[i].data[j].x - Graph::minX) * scaleX;
                    y = height - (Graph::graphVoltage[i].data[j].y - Graph::minYCurrent) * scaleY2;
                    yRef = height - (Graph::graphVoltage[0].data[j].y - Graph::minYCurrent) * scaleY2;
                    if (yRef * 1.5 > y && yRef * .5 < y) {
                        // diameter punt
                        cairo_set_line_width(cr, 5);
                        // vorm instellen
                        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                        // punt plaatsen
                        cairo_move_to(cr, x, y); cairo_line_to(cr, x, y);
                        // kleur plaatsen
                        gdk_cairo_set_source_color(cr, &colorsV);
                        cairo_stroke(cr);
                    }
                }                
            }
        }
        return false;
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void start_calibration(GtkWidget* widget, gpointer user_data) {
        calibrationRequested = true;
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
    void measurefirst(GtkWidget* widget, gpointer user_data) {
        // TODO use current component to determine correct graph type for NMOS/PMOS/JFET/...
        Graph::graphType = GraphType::IB_IC;
        measurementRequested = true;
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void measuresecond(GtkWidget* widget, gpointer user_data) {
        // TODO use current component to determine correct graph type for NMOS/PMOS/JFET/...
        Graph::graphType = GraphType::VCE_IC;
        measurementRequested = true;
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void measurethird(GtkWidget* widget, gpointer user_data) {
        // TODO use current component to determine correct graph type for NMOS/PMOS/JFET/...
        Graph::graphType = GraphType::VBE_IC;
        measurementRequested = true;
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void on_increment_samples(GtkWidget* widget, gpointer user_data) {
        char buffer[25];
        sprintf(buffer, "%d", (++MeasureProperties::currentValueInt[0]));
        gtk_label_set_text(MeasureProperties::currentValue[0], buffer);
        if (MeasureProperties::currentValueInt[0] == UPPER_SAMPLE_LIMIT) {
            gtk_widget_set_sensitive(GTK_WIDGET(MeasureProperties::incrementValue[0]), FALSE);
        }
        gtk_widget_set_sensitive(GTK_WIDGET(MeasureProperties::decrementValue[0]), TRUE);
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void on_decrement_samples(GtkWidget* widget, gpointer user_data) {
        char buffer[25];
        sprintf(buffer, "%d", (--MeasureProperties::currentValueInt[0]));
        gtk_label_set_text(MeasureProperties::currentValue[0], buffer);
        if (MeasureProperties::currentValueInt[0] == LOWER_SAMPLE_LIMIT) {
            gtk_widget_set_sensitive(GTK_WIDGET(MeasureProperties::decrementValue[0]), FALSE);
        }
        gtk_widget_set_sensitive(GTK_WIDGET(MeasureProperties::incrementValue[0]), TRUE);
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void on_increment_points(GtkWidget* widget, gpointer user_data) {
        char buffer[25];
        sprintf(buffer, "%d", (++MeasureProperties::currentValueInt[1]));
        gtk_label_set_text(MeasureProperties::currentValue[1], buffer);
        if (MeasureProperties::currentValueInt[1] == UPPER_POINT_LIMIT) {
            gtk_widget_set_sensitive(GTK_WIDGET(MeasureProperties::incrementValue[1]), FALSE);
        }
        gtk_widget_set_sensitive(GTK_WIDGET(MeasureProperties::decrementValue[1]), TRUE);
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void on_decrement_points(GtkWidget* widget, gpointer user_data) {
        char buffer[25];
        sprintf(buffer, "%d", (--MeasureProperties::currentValueInt[1]));
        gtk_label_set_text(MeasureProperties::currentValue[1], buffer);
        if (MeasureProperties::currentValueInt[1] == LOWER_POINT_LIMIT) {
            gtk_widget_set_sensitive(GTK_WIDGET(MeasureProperties::decrementValue[1]), FALSE);
        }
        gtk_widget_set_sensitive(GTK_WIDGET(MeasureProperties::incrementValue[1]), TRUE);
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void on_sample_voltage_toggle(GtkWidget* widget, gpointer user_data) {
        MeasureProperties::shouldSampleVoltage = !MeasureProperties::shouldSampleVoltage;
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void get_theme(GtkWidget* widget) {
        GtkStyle* style = gtk_widget_get_style(widget);
        if (style != nullptr) {
            colorsC = style->text_aa[GTK_STATE_NORMAL];
            colorsV = style->text[GTK_STATE_NORMAL];
        }
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void help_signal(GtkWidget* widget, gpointer user_data) {
        gtk_widget_show((GtkWidget*) helpDialog.self);
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void close_help_signal(GtkWidget* widget, gpointer user_data) {
        gtk_widget_hide((GtkWidget*) helpDialog.self);
    }
    #ifdef WINDOWS
    G_MODULE_EXPORT
    #endif
    void save_signal(GtkWidget* widget, gpointer user_data) {
        saveRequested = true;
    }
}

void determineType() {
    DUTInformation dut;
    delete Component::currentComponent;
    Component::currentComponent = nullptr;
    // check if DUT is a BJT NPN transistor
    dut = BjtNpn::checkIfNPN();
    if (dut.isSuggestedType) {
        Component::type = BJT_NPN;
        Component::currentComponent = new BjtNpn(dut.orientation);
        return;
    }
    // check if DUT is a BJT PNP transistor
    dut = BjtPnp::checkIfPNP();
    if (dut.isSuggestedType) {
        Component::type = BJT_PNP;
        Component::currentComponent = new BjtPnp(dut.orientation);
        return;
    }
    // check if DUT is a diode
    dut = Diode::checkIfDiode();
    if (dut.isSuggestedType) {
        Component::type = DIODE;
        Component::currentComponent = new Diode(dut.orientation);
        return;
    }
    // check if DUT is a resistor
    dut = Resistor::checkIfResistor();
    if (dut.isSuggestedType) {
        Component::type = RESISTOR;
        Component::currentComponent = new Resistor(dut.orientation);
        return;
    }
    // check if DUT is a capacitor
    //TODO

    //TODO MOSFET_NMOS, MOSFET_PMOS, MOSFET_JFET
    Component::type = UNKNOWN_DEVICE;
}

void UserInterface::init(int* argc, char *** argv) {
    // start probes
    Probe::init();

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

    for (unsigned char i = 0; i < 2; ++i) {
        mainWindow.topPanel.settings.description[i] = GTK_LABEL(gtk_builder_get_object(builder, MeasureProperties::descriptionNames[i]));
        mainWindow.topPanel.settings.currentValue[i] = GTK_LABEL(gtk_builder_get_object(builder, MeasureProperties::currentValueNames[i]));
        mainWindow.topPanel.settings.incrementValue[i] = GTK_BUTTON(gtk_builder_get_object(builder, MeasureProperties::incrementValueNames[i]));
        mainWindow.topPanel.settings.decrementValue[i] = GTK_BUTTON(gtk_builder_get_object(builder, MeasureProperties::decrementValueNames[i]));
    }

    mainWindow.topPanel.settings.sampleVoltageButton = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, MeasureProperties::sampleVoltageButtonLabel));

    mainWindow.topPanel.settings.currentValueInt[0] = 3;
    mainWindow.topPanel.settings.currentValueInt[1] = 50;

    for (unsigned char i = 0; i < 4; ++i) {
        mainWindow.topPanel.properties.property[i] = GTK_LABEL(gtk_builder_get_object(builder, ComponentProperties::propertyNames[i]));
    }

    mainWindow.topPanel.component.componentName = GTK_LABEL(gtk_builder_get_object(builder, "component_name"));

    for (unsigned char i = 0; i < 3; ++i) {
        mainWindow.topPanel.component.pinout[i] = GTK_LABEL(gtk_builder_get_object(builder, ComponentLayout::pinoutNames[i]));
    }

    mainWindow.topPanel.component.symbol = GTK_IMAGE(gtk_builder_get_object(builder, "component_symbol"));

    mainWindow.topPanel.help = GTK_BUTTON(gtk_builder_get_object(builder, "help"));
    mainWindow.topPanel.save = GTK_BUTTON(gtk_builder_get_object(builder, "save"));

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

    helpDialog.self = GTK_DIALOG(gtk_builder_get_object(builder, "help_dialog"));
    helpDialog.close = GTK_BUTTON(gtk_builder_get_object(builder, "close_help"));
    helpDialog.title = GTK_LABEL(gtk_builder_get_object(builder, "help_title"));

    {
        char titleBuffer[50];
        sprintf(titleBuffer, "Transistortester uitleg - %s", VERSION);
        gtk_label_set_text(helpDialog.title, titleBuffer);
    }
    
    // make it so the close button cant be pressed without calibrating first
    gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.dialog_close_button, FALSE);
    mainWindow.topPanel.disableSaveKey();
    mainWindow.disableAllButtons();
    programAlive = true;
    backgroundThread = std::thread([]() {
        while (programAlive) {
            if (calibrationRequested) {
                g_idle_add(G_SOURCE_FUNC(+[]() {
                    gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.dialog_close_button, FALSE);
                    gtk_widget_set_sensitive((GtkWidget*) calibrationDialog.start_button, FALSE);
                    return FALSE;
                }), NULL);
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
                mainWindow.topPanel.enableButtons();
                mainWindow.topPanel.disableSaveKey();
                calibrationRequested = false;
            }
            if (determenRequested) {
                mainWindow.disableAllButtons();
                GraphWindow::isStable = false;
                determineType();
                mainWindow.topPanel.component.update();
                if (Component::type == BJT_NPN || Component::type == BJT_PNP) {
                    mainWindow.bottomPanel.updateButtons();
                }
                mainWindow.topPanel.properties.update();
                mainWindow.topPanel.enableButtons();
                mainWindow.topPanel.disableSaveKey();
                determenRequested = false;
            }
            if (measurementRequested) {
                mainWindow.disableAllButtons();
                GraphWindow::isStable = false;
                switch (Component::type) {
                    case BJT_NPN: {
                        switch (Graph::graphType) {
                            case (GraphType::IB_IC): {
                                ((BjtNpn*) Component::currentComponent)->generateIbIcGraph(mainWindow.topPanel.settings.currentValueInt[1], mainWindow.topPanel.settings.currentValueInt[0], mainWindow.topPanel.settings.shouldSampleVoltage);
                                break;
                            }
                            case (GraphType::VCE_IC): {
                                ((BjtNpn*) Component::currentComponent)->generateVceIcGraph(mainWindow.topPanel.settings.currentValueInt[1], mainWindow.topPanel.settings.currentValueInt[0]);
                                break;
                            }
                            case (GraphType::VBE_IC): {
                                ((BjtNpn*) Component::currentComponent)->generateVbeIcGraph(mainWindow.topPanel.settings.currentValueInt[1], mainWindow.topPanel.settings.currentValueInt[0]);
                                break;
                            }
                        }                                
                        mainWindow.bottomPanel.graphWindow.updateGraph();
                        mainWindow.bottomPanel.updateButtons();
                        mainWindow.topPanel.enableSaveKey();
                        break;
                    }
                    case BJT_PNP: {
                        switch (Graph::graphType) {
                            case (GraphType::IB_IC): {
                                ((BjtPnp*) Component::currentComponent)->generateIbIcGraph(mainWindow.topPanel.settings.currentValueInt[1], mainWindow.topPanel.settings.currentValueInt[0], mainWindow.topPanel.settings.shouldSampleVoltage);
                                break;
                            }
                            case (GraphType::VCE_IC): {
                                ((BjtPnp*) Component::currentComponent)->generateVceIcGraph(mainWindow.topPanel.settings.currentValueInt[1], mainWindow.topPanel.settings.currentValueInt[0]);
                                break;
                            }
                            case (GraphType::VBE_IC): {
                                ((BjtPnp*) Component::currentComponent)->generateVbeIcGraph(mainWindow.topPanel.settings.currentValueInt[1], mainWindow.topPanel.settings.currentValueInt[0]);
                                break;
                            }
                        }                                
                        mainWindow.bottomPanel.graphWindow.updateGraph();
                        mainWindow.bottomPanel.updateButtons();
                        mainWindow.topPanel.enableSaveKey();
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
                mainWindow.topPanel.enableButtons();
                measurementRequested = false;
            }
            if (saveRequested) {
                // knoppen afleggen
                mainWindow.disableAllButtons();
                // naam bestand en component bijhouden
                char buffer[100], subbuffer[50];
                // bestandsnaam vormen adhv huidige component & tijd
                // bron: https://www.tutorialspoint.com/cplusplus/cpp_date_time.htm
                time_t now = time(0);
                tm* lt = localtime(&now);
                Component::currentComponent->getPropertyText(PropertyType::TYPE_NAME, subbuffer);
                sprintf(buffer, "%s_%i-%i-%i_%i:%i:%i.csv", subbuffer, lt->tm_mday, 1 + lt->tm_mon, 1900 + lt->tm_year, lt->tm_hour, lt->tm_min, lt->tm_sec);
                // bestand maken
                std::ofstream output(buffer);
                // eerste regel schrijven
                output << "TRANSISTORTESTER " << VERSION << CSV_DELIMITER <<
                "INGO CHIN & TOM WINDELS" << CSV_DELIMITER <<
                "MEETRESULTATEN " << subbuffer << CSV_DELIMITER <<
                GraphContext::data[Graph::graphType].graphTitle << CSV_DELIMITER <<
                CSV_DELIMITER <<
                "DATUM: " << lt->tm_mday << "/" << 1 + lt->tm_mon << "/" << 1900 + lt->tm_year << '\n';
                // tweede regel schrijven
                output << '[' << GraphContext::data[Graph::graphType].xUnit << ']' << CSV_DELIMITER <<
                "[" << GraphContext::data[Graph::graphType].yUnit1 << ']' << CSV_DELIMITER <<
                "[" << GraphContext::data[Graph::graphType].xUnit << ']' << CSV_DELIMITER <<
                "[" << GraphContext::data[Graph::graphType].yUnit1 << ']' << CSV_DELIMITER <<
                "[" << GraphContext::data[Graph::graphType].xUnit << ']' << CSV_DELIMITER <<
                "[" << GraphContext::data[Graph::graphType].yUnit1 << ']' << '\n';
                // data invullen
                for (unsigned int i = 0; i < Graph::nPoints; ++i) {
                    output << Graph::graphCurrent[0].data[i].x / GraphContext::data[Graph::graphType].scaleFactorX << CSV_DELIMITER << 
                    Graph::graphCurrent[0].data[i].y / GraphContext::data[Graph::graphType].scaleFactorY1 << CSV_DELIMITER <<
                    Graph::graphCurrent[1].data[i].x / GraphContext::data[Graph::graphType].scaleFactorX << CSV_DELIMITER <<
                    Graph::graphCurrent[1].data[i].y / GraphContext::data[Graph::graphType].scaleFactorY1 << CSV_DELIMITER <<
                    Graph::graphCurrent[2].data[i].x / GraphContext::data[Graph::graphType].scaleFactorX << CSV_DELIMITER <<
                    Graph::graphCurrent[2].data[i].y / GraphContext::data[Graph::graphType].scaleFactorY1 << '\n';
                }
                // controleren indien er ook spanning gemeten werd
                if (GraphContext::data[Graph::graphType].canMeasureVoltage && Graph::graphVoltage[0].data != nullptr) {
                    // opnieuw units uitschrijven, nu voor andere grafiek
                    output << '[' << GraphContext::data[Graph::graphType].xUnit << ']' << CSV_DELIMITER <<
                    "[" << GraphContext::data[Graph::graphType].yUnit2 << ']' << CSV_DELIMITER <<
                    "[" << GraphContext::data[Graph::graphType].xUnit << ']' << CSV_DELIMITER <<
                    "[" << GraphContext::data[Graph::graphType].yUnit2 << ']' << CSV_DELIMITER <<
                    "[" << GraphContext::data[Graph::graphType].xUnit << ']' << CSV_DELIMITER <<
                    "[" << GraphContext::data[Graph::graphType].yUnit2 << ']' << '\n';
                    // data invullen
                    for (unsigned int i = 0; i < Graph::nPoints; ++i) {
                        output << Graph::graphVoltage[0].data[i].x / GraphContext::data[Graph::graphType].scaleFactorX << CSV_DELIMITER << 
                        Graph::graphVoltage[0].data[i].y / GraphContext::data[Graph::graphType].scaleFactorY2 << CSV_DELIMITER <<
                        Graph::graphVoltage[1].data[i].x / GraphContext::data[Graph::graphType].scaleFactorX << CSV_DELIMITER <<
                        Graph::graphVoltage[1].data[i].y / GraphContext::data[Graph::graphType].scaleFactorY2 << CSV_DELIMITER <<
                        Graph::graphVoltage[2].data[i].x / GraphContext::data[Graph::graphType].scaleFactorX << CSV_DELIMITER <<
                        Graph::graphVoltage[2].data[i].y / GraphContext::data[Graph::graphType].scaleFactorY2 << '\n';
                    }
                }
                // bestand sluiten
                output.close();
                // knoppen terug aanleggen
                mainWindow.topPanel.enableButtons();
                mainWindow.bottomPanel.updateButtons();
                saveRequested = false;
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
    Component::type = UNKNOWN_DEVICE;
    Component::currentComponent = nullptr;
    mainWindow.topPanel.component.update();
    mainWindow.topPanel.properties.update();
    // start gtk functionality
    gtk_main();
}

void destroy() {
    programAlive = false;
    backgroundThread.join();
    gtk_main_quit();
    Probe::destroy();
}
