#include "probe.hpp"

int main(int argc, char** argv) {
    Probe::first.setVoltage(0);
    Probe::second.setVoltage(100); // 3832 >> 1
    Probe::third.setVoltage(4095);       // 3832
    std::cin.get();
    std::cout << "First probe measurement: " << std::dec << Probe::first.readVoltage() << " mV.\n";
    std::cout << "Second probe measurement: " << std::dec << Probe::second.readVoltage() << " mV.\n";
    std::cout << "Third probe measurement: " << std::dec << Probe::third.readVoltage() << " mV.\n";
    std::cin.get();
    std::cout << "First probe current measurement: " << std::dec << Probe::first.readCurrent() << " µA.\n";
    std::cout << "Second probe current measurement: " << std::dec << Probe::second.readCurrent() << " µA.\n";
    std::cout << "Third probe current measurement: " << std::dec << Probe::third.readCurrent() << " µA.\n";
    std::cin.get();
    return 0;
}