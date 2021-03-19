#include "calibration_window.hpp"
#ifdef COMPILE_WITH_CALIBRATION_WINDOW
#include <signal.h>// voor callbacks bij het resizen van de terminal
#include <stdio.h> // voor sprintf()
#include <string.h>// voor strlen()

#define MIN_X 85
#define MIN_Y 15

#define CTRL(x) ((x) & 0x1F)

#define CLOSE_KEY "CTRL+X"
#define RESET_KEY "CTRL+R"

#ifndef WINDOWS
bool hasResized = false;

void onResize(int) {
    hasResized = true;
}
#endif

struct Coordinate {
    unsigned int x, y;
    Coordinate() {}
    Coordinate(unsigned int x, unsigned int y) : x(x), y(y) {}
};

struct Label {
    Coordinate start;
    unsigned int length;
    Label() {};
    Label(unsigned int x, unsigned int y, unsigned int l) : start(x, y), length(l) {}
};

struct ProbeField {
    Label dacVoltage;
    Label inaVoltage;
    Label inaCurrent;
};

struct TerminalBuffer {
    static TerminalBuffer* buffer;
    static uint64_t nLinesMAX;
    static uint64_t nLines;

    char* text = nullptr;
};

TerminalBuffer* TerminalBuffer::buffer = nullptr;
uint64_t TerminalBuffer::nLinesMAX = 0;
uint64_t TerminalBuffer::nLines = 0;

ProbeField* probeFields;
uint64_t max_x, max_y;
Coordinate cursorLocation(7, 0);
bool cursorVisible = true; // aangeven of de cursor nu wel of niet zichtbaar is
uint8_t cursorTick = 0; // elke keer wanneer deze 70 groot is, wordt de visibility gealterneerd
WINDOW *cli = nullptr;
WINDOW *cli_inner = nullptr; // deze wordt gecleared om de output te vernieuwen
char currentCommandBuffer[45]; // commands langer dan 45 characters worden niet verwacht
bool probeMode = false;
uint8_t probeNr = 0; // 1 - 3, indien true

void initialiseScreen() {
    clear();

    // tekenen probe schermen adhv eigenschappen schermgrootte
    curs_set(0);
    getmaxyx(stdscr, max_y, max_x);
    if (max_x < MIN_X || max_y < MIN_Y) {
        printw("Het terminalscherm is te klein!\nMinimaal %d lijnen en %d elementen nodig.\nNu is dit %d lijnen en %d elementen groot.", MIN_Y, MIN_X, max_y, max_x);
        move(max_y - 1, 0);
        #ifdef WINDOWS
        printw("Gelieve het scherm te stoppen door op '%s' te drukken en daarna het scherm te vergroten.", CLOSE_KEY);
        #else
        printw("Gelieve het scherm te vergroten en op '%s' te drukken, of het scherm te stoppen door op '%s' te drukken.", RESET_KEY, CLOSE_KEY);
        #endif
        refresh();
        keypad(stdscr, true);
        if (cli) {
            keypad(cli, false);
            delwin(cli);
            cli = nullptr;
        }
        return;
    }

    uint64_t t = (max_x - 12) / 3 - 2;
    uint64_t r = 5 + t;
    uint64_t s = (max_x - 12) / 6;
    uint64_t k;
    for (uint8_t i = 0; i < 3; ++i) {
        k = r * i + 4;
        probeFields[i].dacVoltage.length = t;
        probeFields[i].inaVoltage.length = t;
        probeFields[i].inaCurrent.length = t;

        probeFields[i].dacVoltage.start.x = k;
        probeFields[i].inaVoltage.start.x = k;
        probeFields[i].inaCurrent.start.x = k;

        probeFields[i].dacVoltage.start.y = 4;
        probeFields[i].inaVoltage.start.y = 5;
        probeFields[i].inaCurrent.start.y = 6;

        WINDOW *probeEdges = newwin(5, t + 2, 3, r * i + 3);
        box(probeEdges, 0, 0);
        refresh();
        wrefresh(probeEdges);
        attron(A_REVERSE);
        // move(3, s * (2 * i + 1) - 2 + 3 * i); // oude stijl, Probe-titel gecentreerd
        move(3, r * i + 5); // nieuwe stijl, Probe-titel links uitgelijnd
        printw(" Probe %d ", i + 1);
        attroff(A_REVERSE);
        delwin(probeEdges);

        // initiele dac-spanningen tonen
        move(probeFields[i].dacVoltage.start.y, probeFields[i].dacVoltage.start.x);
        printw("DAC: %*d mV", probeFields[i].dacVoltage.length - 8, Probe::probe[i].currentVoltageSet);
    }

    // tekenen en initialiseren CLI-scherm

    if (cli != nullptr) {
        delwin(cli);
        delwin(cli_inner);
    } else {
        keypad(stdscr, false);
    }
    // if (terminalBuffer != nullptr) {
    //     delete[] terminalBuffer;
    // }
    // nTerminalLines = 0;
    // terminalBuffer = new char[(max_y - 12) * (max_x - 7)];
    // currentIndex = 0;

    if (TerminalBuffer::buffer != nullptr) {
        delete[] TerminalBuffer::buffer;
    }
    TerminalBuffer::nLinesMAX = max_y - 13;
    TerminalBuffer::nLines = 0;
    TerminalBuffer::buffer = new TerminalBuffer[TerminalBuffer::nLinesMAX + 20]; // beetje extra plaats voorzien, zie verder bij de '\n' input

    cli = newwin(max_y - 11, max_x - 6, 9, 3);
    cli_inner = newwin(max_y - 13, max_x - 8, 10, 4);
    box(cli, 0, 0);
    refresh();
    wrefresh(cli);
    attron(A_REVERSE);
    move(9, 5);
    printw(" Terminal ");
    attroff(A_REVERSE);
    keypad(cli, true);
    nodelay(cli, true); // wgetch() is een non-blocking call nu

    cursorLocation.y = max_y - 4;
    cursorLocation.x = 7;

    move(max_y - 4, 5);
    if (probeMode) {
        printw("Probe %d#");
    } else {
        printw("$");
    }
    
    // tekenen rand-informatie

    //   titel
    move(0, 0);
    attron(A_REVERSE);
    #ifdef WINDOWS
    printw(" Transistortester kalibratiescherm (gecompileerd voor Windows)%*s ", max_x - 63, "Ingo Chin, Tom Windels");
    #else
    #ifdef USING_RPI
    printw(" Transistortester kalibratiescherm (gecompileerd voor Raspberry Pi)%*s ", max_x - 68, "Ingo Chin, Tom Windels");
    #else
    printw(" Transistortester kalibratiescherm%*s ", max_x - 35, "Ingo Chin, Tom Windels");
    #endif
    #endif
    //    toets-balk
    move(max_y - 1, 0);
    printw("^X          ^R %*s ", max_x - 16, VERSION);
    attroff(A_REVERSE);
    move(max_y - 1, 3);
    printw(" Sluiten ");
    move(max_y - 1, 15);
    printw(" Herstarten ");
}

int interpretNumber(const char * str, uint64_t* errorIndex) {
    int64_t result = 0;
    uint64_t j = 0;
    uint64_t len = strlen(str);
    while (str[j] == '\0') ++j;
    for (uint64_t i = j; i < len; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            result *= 10;
            result += str[i] - '0';
        } else {
            *errorIndex = i;
            return -1;
        }
    }
    return result;
}

char* interpretCommand(const char * cmd) {
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {                    
        char* returnText = new char[450]; // tijdelijke buffer die de volledige output bevat van meerdere lijnen
        if (!probeMode) {
            sprintf(returnText,
            "---\n"
            "Transistortester - %s\n"
            "---\n"
            "  p, probe <1-3>      \tGaat over naar probe-modus.\n"
            "  e, exit             \tSluit het kalibratiescherm.\n"
            "  clr, clear          \tMaakt het scherm leeg.\n"
            "  h, help             \tToont deze help\n"
            "  s, save             \tSla de huidige configuratie op.\n"
            "---"
            , VERSION);
        } else {
            sprintf(returnText,
            "---\n"
            "Transistortester - Probemode actief - %s\n"
            "---\n"
            "  v, voltage <waarde>[mV]\tStel <waarde> mV in op de probe.\n"
            "  s, shunt <waarde>[mOhm]\tStel <waarde> in als shuntweerstand.\n"
            "  cu, current <waarde>[uA]\tStel <waarde> in als verwachte stroom door de probe.\n"
            "  o, offset <waarde>[mV]\tStel <waarde> in als *constante* offset.\n"
            "  ca, calibrate        \tHerkalibreer (nuttig wanneer er een offset is ingesteld).\n"
            "  e, exit             \tStop probemode.\n"
            "---"
            , VERSION);
        }
        return returnText;
    } else if (cmd[0] == 'v') {
        uint64_t verkeerdeIndex; // gebruikt voor errortekst indien nodig
        int64_t voltage;
        if (cmd[1] == ' ') {
            if (probeMode) {
                voltage = interpretNumber(&cmd[2], &verkeerdeIndex);
            } else {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
        } else {
            for (uint8_t i = 1; i < 8; ++i) {
                if (cmd[i] != "voltage "[i]) { // dit werkt blijkbaar?
                    goto niet_herkend;
                }
            }
            if (probeMode) {
                voltage = interpretNumber(&cmd[8], &verkeerdeIndex);
            } else {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
        }
        if (voltage == -1) {
            char* returnText = new char[150];
            sprintf(returnText,
            "Ongekende spanning: %s.\n"
            "Gelieve geen eenheden mee te geven, maar telkens de gewenste waarde in mV uit te drukken."
            , &cmd[verkeerdeIndex]);
            return returnText;
        } else if (voltage <= 5000) {
            // spanning instellen
            Probe::probe[probeNr - 1].setVoltage(voltage);
            // aangepaste spanning tonen
            move(probeFields[probeNr - 1].dacVoltage.start.y, probeFields[probeNr - 1].dacVoltage.start.x);
            printw("DAC: %*d mV", probeFields[probeNr - 1].dacVoltage.length - 8, Probe::probe[probeNr - 1].currentVoltageSet);
            char* returnText = new char[65];
            sprintf(returnText,
            "De spanning voor probe %d is nu ingesteld op %d mV!"
            , probeNr, voltage);
            return returnText;
        } else {
            char* returnText = new char[100];
            sprintf(returnText,
            "Spanning %d mV is te groot. Maximumspanning bedraagd 5000mV."
            , voltage);
            return returnText;
        }
    } else if (cmd[0] == 's') {
        uint64_t verkeerdeIndex;
        int64_t shuntWaarde;
        if (cmd[1] == ' ') {
            if (!probeMode) {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
            shuntWaarde = interpretNumber(&cmd[2], &verkeerdeIndex);
        } else {
            for (uint8_t i = 1; i < 6; ++i) {
                if (cmd[i] != "shunt "[i]) {
                    goto niet_herkend;
                }
            }
            if (!probeMode) {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
            shuntWaarde = interpretNumber(&cmd[6], &verkeerdeIndex);
        }
        if (shuntWaarde == -1) {
            char* returnText = new char[150];
            sprintf(returnText,
            "Ongekende shuntwaarde: %s.\n"
            "Gelieve geen eenheden mee te geven, maar telkens de gewenste waarde in mOhm uit te drukken."
            , &cmd[verkeerdeIndex]);
            return returnText;
        } else {
            Probe::probe[probeNr - 1].setShunt(shuntWaarde / 1000.f);
            char* returnText = new char[65];
            sprintf(returnText,
            "De shuntwaarde voor probe %d is nu ingesteld op %d mOhm!"
            , probeNr, shuntWaarde);
            return returnText;
        }
    } else if (cmd[0] == 'c' && cmd[1] == 'u') { // current
        uint64_t verkeerdeIndex;
        int64_t currentWaarde;
        if (cmd[2] == ' ') {
            if (!probeMode) {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
            currentWaarde = interpretNumber(&cmd[3], &verkeerdeIndex);
        } else {
            for (uint8_t i = 2; i < 8; ++i) {
                if (cmd[i] != "current "[i]) {
                    goto niet_herkend;
                }
            }
            if (!probeMode) {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
            currentWaarde = interpretNumber(&cmd[8], &verkeerdeIndex);
        }
        if (currentWaarde == -1) {
            char* returnText = new char[150];
            sprintf(returnText,
            "Ongekende stroom: %s.\n"
            "Gelieve geen eenheden mee te geven, maar telkens de gewenste waarde in uA uit te drukken."
            , &cmd[verkeerdeIndex]);
            return returnText;
        } else {
            uint64_t nieuweShuntWaarde = Probe::probe[probeNr - 1].adjustShuntUsingCurrent(currentWaarde) * 1000;
            char* returnText = new char[150];
            sprintf(returnText,
            "De shuntwaarde voor probe %d is nu zodanig geregeld tot er %d uA gemeten wordt.\n"
            "De nieuwe shuntwaarde bedraagt nu %d mOhm."
            , probeNr, currentWaarde, nieuweShuntWaarde);
            return returnText;
        }
    } else if (cmd[0] == 'o') { // offset
        uint64_t verkeerdeIndex;
        int64_t offsetWaarde;
        if (cmd[1] == ' ') {
            if (!probeMode) {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
            offsetWaarde = interpretNumber(&cmd[2], &verkeerdeIndex);
        } else {
            for (uint8_t i = 2; i < 7; ++i) {
                if (cmd[i] != "offset "[i]) {
                    goto niet_herkend;
                }
            }
            if (!probeMode) {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
            offsetWaarde = interpretNumber(&cmd[7], &verkeerdeIndex);
        }
        if (offsetWaarde == -1) {
            char* returnText = new char[150];
            sprintf(returnText,
            "Ongekende offset: %s.\n"
            "Gelieve geen eenheden mee te geven, maar telkens de gewenste waarde in mV uit te drukken."
            , &cmd[verkeerdeIndex]);
            return returnText;
        } else {
            //TODO offset instellen in probe [probeNr-1]
            char* returnText = new char[90];
            sprintf(returnText,
            "De offset voor probe %d is nu %d mV."
            , probeNr, offsetWaarde);
            return returnText;
        }
    } else if (cmd[0] == 'c' && cmd[1] == 'a') { // kalibratie
        uint64_t len = strlen(cmd);
        if (len == 2) {
            if (!probeMode) {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
            Probe::probe[probeNr - 1].calibrate();
            char* returnText = new char[60];
            sprintf(returnText, "Probe %d is opnieuw gekalibreerd.", probeNr);
            return returnText;
        } else {
            for (uint8_t i = 2; i < 10; ++i) {
                if (cmd[i] != "calibrate"[i]) {
                    goto niet_herkend;
                }
            }
            if (!probeMode) {
                char* returnText = new char[60];
                strcpy(returnText, "Gelieve eerst een probe te selecteren via p, probe <1-3>.");
                return returnText;
            }
            Probe::probe[probeNr - 1].calibrate();
            char* returnText = new char[60];
            sprintf(returnText, "Probe %d is opnieuw gekalibreerd.", probeNr);
            return returnText;
        }
    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "e") == 0) {
        if (probeMode) {
            char* returnText = new char[15];
            sprintf(returnText, "Probemode is gestopt.");
            probeMode = false;
            return returnText;
        }
        CalibrationWindow::shouldExit = true;
        return nullptr;
    } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "clr") == 0) {
        initialiseScreen();
        return nullptr;
    } else if (cmd[0] == 'p') {
        if (strlen(cmd) == 3 && cmd[1] == ' ') {
            if (cmd[2] == '1') {
                goto probe1;
            } else if (cmd[2] == '2') {
                goto probe2;
            } else if (cmd[2] == '3') {
                goto probe3;
            } else {
                char* returnText = new char[40];
                sprintf(returnText, "Probenummer %d is niet correct.", cmd[2] - '0');
                return returnText;
            }
        } else if (strlen(cmd) == 7) {
            for (uint8_t i = 1; i < 6; ++i) {
                if (cmd[i] != "probe "[i]) {
                    goto niet_herkend;
                }
            }
            if (cmd[6] == '1') {
                probe1:
                char* returnText = new char[35];
                sprintf(returnText, "Overschakelen naar probe 1");
                probeMode = true;
                probeNr = 1;
                return returnText;
            } else if (cmd[6] == '2') {
                probe2:
                char* returnText = new char[35];
                sprintf(returnText, "Overschakelen naar probe 2");
                probeMode = true;
                probeNr = 2;
                return returnText;
            } else if (cmd[6] == '3') {
                probe3:
                char* returnText = new char[35];
                sprintf(returnText, "Overschakelen naar probe 3");
                probeMode = true;
                probeNr = 3;
                return returnText;
            } else {
                char* returnText = new char[35];
                sprintf(returnText, "Probenummer %d is niet correct.", cmd[6] - '0');
                return returnText;
            }
        } else {
            goto niet_herkend;
        }
        
    }
    niet_herkend:
    char* returnText = new char[100]; // cmd zelf kan vrij groot zijn
    sprintf(returnText, "Command '%s' werd niet herkend!", cmd);
    return returnText;
}

namespace CalibrationWindow {

    bool shouldExit = false;

    void init() {
        initscr();
        raw();
        noecho();
        nonl();
        timeout(0);
        probeFields = new ProbeField[3];
        #ifndef WINDOWS
        signal(SIGWINCH, onResize);
        #endif
        currentCommandBuffer[44] = '\0';
        initialiseScreen();
    }

    void destroy() {
        endwin();
        delete[] probeFields;
        // delete[] terminalBuffer;
        delete[] TerminalBuffer::buffer;
    }

    void update() {
        #ifndef WINDOWS
        if (hasResized) {
            hasResized = false;
            endwin();
            initscr();
            raw();
            noecho();
            nonl();
            initialiseScreen();
        }
        #endif
        if (max_x >= MIN_X && max_y >= MIN_Y) {
            for (uint8_t i = 0; i < 3; ++i) {
                // move(probeFields[i].dacVoltage.start.y, probeFields[i].dacVoltage.start.x);
                // printw("DAC: %*d mV", probeFields[i].dacVoltage.length - 8, Probe::probe[i].currentVoltageSet);
                move(probeFields[i].inaVoltage.start.y, probeFields[i].inaVoltage.start.x);
                printw("INA: %*d mV", probeFields[i].inaVoltage.length - 8, Probe::probe[i].readVoltage());
                move(probeFields[i].inaCurrent.start.y, probeFields[i].inaCurrent.start.x);
                printw("%*f uA", probeFields[i].inaCurrent.length - 3, Probe::probe[i].readCurrent());
                refresh();
            }
        }
        int c = cli ? wgetch(cli) : wgetch(stdscr);
        switch (c) {
            case CTRL('x'): {
                endwin();
                initscr();
                raw();
                noecho();
                shouldExit = true;
                break;
            }
            case CTRL('r'): {
                initialiseScreen();
                break;
            }
            case '\n': // newline, 13 komt ook op sommige systemen voor als de enterknop
            case 13: {
                // de buffer "afsluiten" met \0, waardoor de buffer als string kan worden geinterpreteerd
                if (!probeMode) {
                    currentCommandBuffer[cursorLocation.x - 7] = '\0';
                } else {
                    currentCommandBuffer[cursorLocation.x - 14] = '\0';
                }

                // er kunnen maar nLinesMAX tergelijk worden getoond,
                // het geheugen wordt maar bij 8 extra entries verplaatst
                // zodanig dat het aantal keer verplaatsen van geheugen
                // beperkter is

                if (TerminalBuffer::nLines > TerminalBuffer::nLinesMAX + 15) {
                    // nieuwe buffer maken die gedeeltelijk wordt opgevuld
                    TerminalBuffer* newBuffer = new TerminalBuffer[TerminalBuffer::nLinesMAX + 20];
                    // plaats van het "oudste" commando dat nog wordt weergegeven vinden
                    const TerminalBuffer* start = &TerminalBuffer::buffer[20];
                    // vanaf hier kopieren uit de oude buffer
                    for (uint8_t i = 0; i < TerminalBuffer::nLinesMAX; ++i) {
                        newBuffer[i].text = start[i].text;
                    }
                    // aanduiden in nLines dat de buffer nu weer kleiner is, nl. het maximum aantal weergegeven
                    TerminalBuffer::nLines = TerminalBuffer::nLinesMAX - 1;
                    // oude "sub"buffers vrijgeven
                    for (uint8_t i = 0; i < 20; ++i) {
                        delete[] TerminalBuffer::buffer[i].text;
                    }
                    // oude buffer vrijgeven
                    delete[] TerminalBuffer::buffer;
                    // nieuwe buffer gebruiken
                    TerminalBuffer::buffer = newBuffer;
                }

                // de buffer als string interpreteren, en toevoegen aan de terminalbuffer
                uint8_t len = strlen(currentCommandBuffer) + 2; // er komt 2 bij wegens de tekens "> "
                TerminalBuffer::buffer[TerminalBuffer::nLines].text = new char[len + 1]; // "sub"geheugen aanmaken
                TerminalBuffer::buffer[TerminalBuffer::nLines].text[0] = probeMode? '%' : '>'; // '>' & ' ' invoegen
                TerminalBuffer::buffer[TerminalBuffer::nLines].text[1] = ' ';
                for (uint8_t i = 2; i < len; ++i) { // commando zelf in buffer steken
                    TerminalBuffer::buffer[TerminalBuffer::nLines].text[i] = currentCommandBuffer[i - 2];
                }
                TerminalBuffer::buffer[TerminalBuffer::nLines].text[len] = '\0'; // "sub"buffer "sluiten"
                ++TerminalBuffer::nLines;

                // de buffer als ingegeven commando interpreteren
                char* returnStatement = interpretCommand(currentCommandBuffer);
                // indien er een return statement is, is returnStatement niet de nullptr
                if (returnStatement) {
                    uint64_t index = 0;
                    uint64_t previousNewline = 0;
                    while (returnStatement[index]) {
                        if (returnStatement[index] == '\n') {
                            TerminalBuffer::buffer[TerminalBuffer::nLines].text = new char[index - previousNewline + 1];
                            for (uint64_t i = previousNewline; i < index; ++i) {
                                TerminalBuffer::buffer[TerminalBuffer::nLines].text[i - previousNewline] = returnStatement[i];
                            }
                            TerminalBuffer::buffer[TerminalBuffer::nLines].text[index - previousNewline] = '\0';
                            previousNewline = index + 1;
                            ++TerminalBuffer::nLines;
                        }
                        ++index;
                    }
                    TerminalBuffer::buffer[TerminalBuffer::nLines].text = new char[index - previousNewline];
                    strcpy(TerminalBuffer::buffer[TerminalBuffer::nLines].text, returnStatement + previousNewline);
                    ++TerminalBuffer::nLines;
                    delete[] returnStatement;
                }
                // alle (zichtbare) lijnen op het scherm plaatsen
                // eerst huidige zichtbare lijnen wissen
                werase(cli_inner);
                wrefresh(cli_inner);
                // nieuwe lijnen plaatsen
                uint64_t nPrint = TerminalBuffer::nLines > TerminalBuffer::nLinesMAX - 1 ? TerminalBuffer::nLinesMAX : TerminalBuffer::nLines + 1;
                for (uint8_t i = 1; i < nPrint; ++i) {
                    move(max_y - 4 - i, 5);
                    printw("%s", TerminalBuffer::buffer[TerminalBuffer::nLines - i].text);
                }
                move(max_y - 4, 5);
                if (probeMode) {
                    attron(A_REVERSE);
                    printw("Probe %d#", probeNr);
                    attroff(A_REVERSE);
                    cursorLocation.x = 14;
                } else {
                    printw("$ ");
                    cursorLocation.x = 7;
                }
                break;
            }
            case KEY_BACKSPACE: // backspace, 8 komt ook soms op sommige systemen voor
            case 8: {
                if (!probeMode && cursorLocation.x >= 8 || probeMode && cursorLocation.x >= 15) {
                    move(cursorLocation.y, cursorLocation.x -= 1);
                    printw("  "); // 2 spaties wegens een mogelijke cursor die anders achter blijft
                }
                break;
            }
            case -1: {
                // geen userinput ontvangen
                break;
            }
            default: {
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ') {
                    if (cursorLocation.x < 51) {
                        move(cursorLocation.y, cursorLocation.x++);
                        printw("%c", (char) c);
                        if (!probeMode) {
                            currentCommandBuffer[cursorLocation.x - 8] = c;
                        } else {
                            currentCommandBuffer[cursorLocation.x - 15] = c;
                        }
                    }
                }
                break;
            }
        }
        cursorTick += 1;
        if (cursorTick == 70) {
            cursorTick = 0;
            cursorVisible = !cursorVisible;
        }
        move(cursorLocation.y, cursorLocation.x);
        if (cursorVisible)
            attron(A_REVERSE);
        printw(" ");
        attroff(A_REVERSE);
    }
}
#endif