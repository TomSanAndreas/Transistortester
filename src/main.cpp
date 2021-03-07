#include "probe.hpp"

int main(int argc, char** argv) {
    Probe::first.setVoltage(MIN_VOLTAGE);
    Probe::second.setVoltage(MAX_VOLTAGE >> 1);
    Probe::third.setVoltage(MAX_VOLTAGE);
    std::cout << "First probe measurement: " << Probe::first.readVoltage() << " mV.\n";
    std::cout << "Second probe measurement: " << Probe::second.readVoltage() << " mV.\n";
    std::cout << "Third probe measurement: " << Probe::third.readVoltage() << " mV.\n";
    return 0;
}