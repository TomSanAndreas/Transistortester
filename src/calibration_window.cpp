#include "calibration_window.hpp"
#ifdef COMPILE_WITH_CALIBRATION_WINDOW
#include <signal.h>
#include <time.h>
#include <errno.h>

#define MIN_X 80
#define MIN_Y 30

#ifndef WINDOWS
bool hasResized = false;

void onResize(int) {
    hasResized = true;
}
#endif

struct Label {
    unsigned int startX, startY;
    unsigned int length;
    Label() {};
    Label(unsigned int x, unsigned int y, unsigned int l) : startX(x), startY(y), length(l) {}
};

struct ProbeField {
    Label dacVoltage;
    Label inaVoltage;
    Label inaCurrent;
};

ProbeField* probeFields;

void initialiseScreen() {
    clear();
    int x, y;
    getmaxyx(stdscr, y, x);

    uint64_t t = (x - 12) / 3 - 2;
    uint64_t r = 5 + t;
    for (uint8_t i = 0; i < 3; ++i) {
        probeFields[i].dacVoltage.length = t;
        probeFields[i].inaVoltage.length = t;
        probeFields[i].inaCurrent.length = t;

        probeFields[i].dacVoltage.startX = 4 + r * i;
        probeFields[i].inaVoltage.startX = 4 + r * i;
        probeFields[i].inaCurrent.startX = 4 + r * i;

        probeFields[i].dacVoltage.startY = 4;
        probeFields[i].inaVoltage.startY = 5;
        probeFields[i].inaCurrent.startY = 6;
    }

    WINDOW *eersteProbeScherm = newwin(5, (x - 12) / 3, 3, 3);
    box(eersteProbeScherm, 0, 0);
    refresh();
    wrefresh(eersteProbeScherm);
    attron(A_REVERSE);
    move(3, (x - 12) / 6 - 2);
    printw(" Probe 1 ");
    attroff(A_REVERSE);
    delwin(eersteProbeScherm);

    WINDOW *tweedeProbeScherm = newwin(5, (x - 12) / 3, 3, 6 + (x - 12) / 3);
    box(tweedeProbeScherm, 0, 0);
    refresh();
    wrefresh(tweedeProbeScherm);
    attron(A_REVERSE);
    move(3, (x - 12) / 2 + 1);
    printw(" Probe 2 ");
    attroff(A_REVERSE);
    delwin(tweedeProbeScherm);

    WINDOW *derdeProbeScherm = newwin(5, (x - 12) / 3, 3, 9 + 2 * (x - 12) / 3);
    box(derdeProbeScherm, 0, 0);
    refresh();
    wrefresh(derdeProbeScherm);
    attron(A_REVERSE);
    move(3, 5 * (x - 12) / 6 + 4);
    printw(" Probe 3 ");
    attroff(A_REVERSE);
    delwin(derdeProbeScherm);
}

namespace CalibrationWindow {

    void init() {
        initscr();
        raw();
        noecho();

        timeout(0);

        probeFields = new ProbeField[3];

        #ifndef WINDOWS
        signal(SIGWINCH, onResize);
        #endif
        int x, y;

        getmaxyx(stdscr, y, x);

        if (x < MIN_X || y < MIN_Y) {
            printw("Het terminalscherm is te klein!\nMinimaal %d lijnen en %d elementen nodig.\nNu is dit %d lijnen en %d elementen groot.", MIN_Y, MIN_X, y, x);
        } else {
            initialiseScreen();
        }
    }

    void destroy() {
        endwin();
        delete[] probeFields;
    }

    void update() {
        #ifndef WINDOWS
        if (hasResized) {
            hasResized = false;
            endwin();
            initscr();
            raw();
            noecho();
            int x, y;
            getmaxyx(stdscr, y, x);
            if (x < MIN_X || y < MIN_Y) {
                clear();
                printw("Het terminalscherm is te klein!\nMinimaal %d lijnen en %d elementen nodig.\nNu is dit %d lijnen en %d elementen groot.", MIN_Y, MIN_X, y, x);
            } else {
                initialiseScreen();
            }
        }
        #endif
        int y, x;
        getmaxyx(stdscr, y, x);
        if (x >= MIN_X && y >= MIN_Y) {
            for (uint8_t i = 0; i < 3; ++i) {
                move(probeFields[i].dacVoltage.startY, probeFields[i].dacVoltage.startX);
                printw("DAC: %*d mV", probeFields[i].dacVoltage.length - 8, Probe::probe[i].currentVoltageSet);
                move(probeFields[i].inaVoltage.startY, probeFields[i].inaVoltage.startX);
                printw("INA: %*d mV", probeFields[i].inaVoltage.length - 8, Probe::probe[i].readVoltage());
                move(probeFields[i].inaCurrent.startY, probeFields[i].inaCurrent.startX);
                printw("%*f mA", probeFields[i].inaCurrent.length - 3, ((float) Probe::probe[i].readCurrent()) / 1000);
            }
        }
    }

    // Stackoverflow: "Is there an alternative sleep function in C to milliseconds?"
    // Slaap voor ms milliseconden.
    int sleep(long ms) {
        timespec ts;
        int res;

        if (ms < 0) {
            errno = EINVAL;
            return -1;
        }

        ts.tv_sec = ms/1000;
        ts.tv_nsec = (ms % 1000) * 1000000;

        do {
            res = nanosleep(&ts, &ts);
        } while (res && errno == EINTR);

        return ms;
    }

    bool shouldExit() {
        char c = getch();
        if (c == 'q') {
            return true;
        }
        return false;
    }
}
#endif