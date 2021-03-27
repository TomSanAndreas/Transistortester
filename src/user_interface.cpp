#include "user_interface.hpp"

Graph graphs[3];
ProbeCombination start = {0, 1};

extern "C" {
    #ifdef WINDOWS
    G_MODULE_EXPORT void destroy_signal(GtkWidget* w, gpointer user_data) {
        UserInterface::destroy();
    }
    G_MODULE_EXPORT gboolean draw_signal(GtkWidget* widget, cairo_t* cr, gpointer data) {
        guint width, height;
        GdkRGBA color { 1, 1, 1, 1 };
        width = gtk_widget_get_allocated_width(widget);
        height = gtk_widget_get_allocated_height(widget);
        for (unsigned char i = 0; i < 3; ++i) {
            for (unsigned int j = 0; j < 50; ++j) {
                cairo_line_to(cr, graphs[i].data[j].x, graphs[i].data[j].y);
            }
        }
        gdk_cairo_set_source_rgba(cr, &color);
        cairo_fill(cr);
        return false;
    }
    #elif USING_RPI
    void destroy_signal(GtkWidget* w, gpointer user_data) {
        UserInterface::destroy();
    }
    gboolean draw_signal(GtkWidget* widget, cairo_t* cr, gpointer data) {
        guint width, height;
        GdkRGBA color { 0.5, 0.5, 0.5, 0.5 };
        width = gtk_widget_get_allocated_width(widget);
        height = gtk_widget_get_allocated_height(widget);
        cairo_arc(cr, width / 2.0, height / 2.0, MIN(width, height) / 2.0, 0, 2*G_PI);
        // gtk_style_context_get_color(gtk_widget_get_style_context(widget), GTK_STATE_FLAG_NORMAL, &color);
        gdk_cairo_set_source_rgba(cr, &color);
        cairo_fill(cr);
        return false;
    }
    #endif
}

void measure() {
    MeasureResult data1, data2;
    Probe::probe[start.second].setVoltage(0);
    for (unsigned int i = 1; i < 51; ++i) {
        Probe::probe[start.first].setVoltage(i * 100);
        sleep(1);
        data1 = Probe::probe[start.first].doFullMeasure(5);
        data2 = Probe::probe[start.second].doFullMeasure(5);
        for (unsigned char j = 0; j < 3; ++j) {
            graphs[j].data[i - 1].x = data1.usedVoltage;
        }
        graphs[0].data[i - 1].y = (data1.avgV - data2.avgV) / data1.avgA;
        graphs[1].data[i - 1].y = (data1.minV - data2.minV) / data1.maxA;
        graphs[2].data[i - 1].y = (data1.maxV - data2.maxV) / data1.minA;
    }
}

void UserInterface::init(int* argc, char *** argv) {
    GtkBuilder* builder;
    GtkWidget* window,* graph;
    gtk_init(argc, argv);
    builder = gtk_builder_new_from_file("../ui/landing_page.xml");
    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    
    gtk_builder_connect_signals(builder, NULL);
    graph = GTK_WIDGET(gtk_builder_get_object(builder, "graph"));
    g_object_unref(builder);
    gtk_widget_show(window);
    gtk_main();
}

void UserInterface::destroy() {
    gtk_main_quit();
}