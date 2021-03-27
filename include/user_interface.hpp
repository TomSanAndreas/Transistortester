#pragma once

#include "base.hpp"
#include "parameters.hpp"
#include "probe.hpp"

#include <gtk/gtk.h>

struct ProbeCombination {
    unsigned char first, second;
};

struct Point {
    int x, y;
};

struct Graph {
    Point data[50];
};

namespace UserInterface {
    void init(int* argc, char*** argv);
    void destroy();
}