#include "calibration_window.hpp"
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

ProbeField probe1, probe2, probe3;

void initialiseScreen() {
    clear();
    int x, y;
    getmaxyx(stdscr, y, x);

    probe1.dacVoltage.length = (x - 12) / 3 - 2;
    probe1.inaVoltage.length = (x - 12) / 3 - 2;
    probe1.inaCurrent.length = (x - 12) / 3 - 2;

    probe1.dacVoltage.startX = 4;
    probe1.inaVoltage.startX = 4;
    probe1.inaCurrent.startX = 4;

    probe1.dacVoltage.startY = 4;
    probe1.inaVoltage.startY = 5;
    probe1.inaCurrent.startY = 6;

    probe2.dacVoltage.length = (x - 12) / 3 - 2;
    probe2.inaVoltage.length = (x - 12) / 3 - 2;
    probe2.inaCurrent.length = (x - 12) / 3 - 2;

    probe2.dacVoltage.startX = 7 + (x - 12) / 3;
    probe2.inaVoltage.startX = 7 + (x - 12) / 3;
    probe2.inaCurrent.startX = 7 + (x - 12) / 3;

    probe2.dacVoltage.startY = 4;
    probe2.inaVoltage.startY = 5;
    probe2.inaCurrent.startY = 6;

    probe3.dacVoltage.length = (x - 12) / 3 - 2;
    probe3.inaVoltage.length = (x - 12) / 3 - 2;
    probe3.inaCurrent.length = (x - 12) / 3 - 2;

    probe3.dacVoltage.startX = 10 + 2 * (x - 12) / 3;
    probe3.inaVoltage.startX = 10 + 2 * (x - 12) / 3;
    probe3.inaCurrent.startX = 10 + 2 * (x - 12) / 3;

    probe3.dacVoltage.startY = 4;
    probe3.inaVoltage.startY = 5;
    probe3.inaCurrent.startY = 6;

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

    Probe *eersteProbe, *tweedeProbe, *derdeProbe;

    void init() {
        initscr();
        raw();
        noecho();

        timeout(0);

        eersteProbe = new Probe(Probe::Number[0]);
        tweedeProbe = new Probe(Probe::Number[1]);
        derdeProbe = new Probe(Probe::Number[2]);
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
        delete eersteProbe;
        delete tweedeProbe;
        delete derdeProbe;
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
            move(probe1.dacVoltage.startY, probe1.dacVoltage.startX);
            printw("DAC: %*d mV", probe1.dacVoltage.length - 8, eersteProbe->currentVoltageSet);
            move(probe1.inaVoltage.startY, probe1.inaVoltage.startX);
            printw("INA: %*d mV", probe1.inaVoltage.length - 8, eersteProbe->readVoltage());
            move(probe1.inaCurrent.startY, probe1.inaCurrent.startX);
            printw("%*f mA", probe1.inaCurrent.length - 3, ((float) eersteProbe->readCurrent()) / 1000);

            move(probe2.dacVoltage.startY, probe2.dacVoltage.startX);
            printw("DAC: %*d mV", probe2.dacVoltage.length - 8, tweedeProbe->currentVoltageSet);
            move(probe2.inaVoltage.startY, probe2.inaVoltage.startX);
            printw("INA: %*d mV", probe2.inaVoltage.length - 8, tweedeProbe->readVoltage());
            move(probe2.inaCurrent.startY, probe2.inaCurrent.startX);
            printw("%*f mA", probe2.inaCurrent.length - 3, ((float)tweedeProbe->readCurrent()) / 1000);

            move(probe3.dacVoltage.startY, probe3.dacVoltage.startX);
            printw("DAC: %*d mV", probe3.dacVoltage.length - 8, derdeProbe->currentVoltageSet);
            move(probe3.inaVoltage.startY, probe3.inaVoltage.startX);
            printw("INA: %*d mV", probe3.inaVoltage.length - 8, derdeProbe->readVoltage());
            move(probe3.inaCurrent.startY, probe3.inaCurrent.startX);
            printw("%*f mA", probe3.inaCurrent.length - 3, ((float) derdeProbe->readCurrent()) / 1000);
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
