#include "dac.hpp"

DAC::DAC(DAC_Address addr) : I2C_Device((byte) addr, 3) {
    // stel GAIN, laag startvoltage en andere eigenschappen in, zie pagina 42
    // als initiele instellingen wordt VDD gebruikt, het apparaat aangelegd, en gain ingesteld op 1 (al maakt deze laatste stap weinig uit volgens de datasheet)
    // resultaat: C2:C0 = 0b010, VREF1:VREF0 = 0b00, PD1:PD0 = 0b00, G = 0b0
    byte configBytes[] { 0b01000000, (byte) (currentVoltage >> 4), (byte) ((currentVoltage << 4) & 0xF0) };
    Buffer configBuffer({configBytes, 3});
    queue(configBuffer);
    send();
}

DAC::~DAC() {
    byte configBytes[] { (byte) (0b00110000 | ((0 >> 4) & 0x0F)), (byte) 0 };
    Buffer configBuffer({configBytes, 2});
    queue(configBuffer);
    send();
}

void DAC::turnOff() {
    // C2:C0 = 0b00x, PD1:PD0 = 0b11
    byte configBytes[] { (byte) (0b00110000 | ((0 >> 4) & 0x0F)), (byte) 0 };
    Buffer configBuffer({configBytes, 2});
    queue(configBuffer);
    send();
}

void DAC::turnOn() {
    byte configBytes[] { 0b01000000, (byte) (currentVoltage >> 4), (byte) ((currentVoltage << 4) & 0xF0) };
    Buffer configBuffer({configBytes, 3});
    queue(configBuffer);
    send();
}

void DAC::setVoltage(UVoltage voltage) {
    if (currentVoltage == voltage) {
        return;
    }
    currentVoltage = voltage;
    byte configBytes[] { (byte) ((currentVoltage >> 8) & 0x0F), (byte) currentVoltage };
    Buffer configBuffer({ configBytes, 2 });
    queue(configBuffer);
    send();
}