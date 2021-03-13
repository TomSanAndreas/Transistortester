#include "dac.hpp"

DAC::DAC(DAC_Address addr) : I2C_Device((byte) addr, 3) {
    // set GAIN, low startvoltage ... and other properties, see page 42
    // set initial settings, use VDD, power up the device and set gain to 1 (don't care when VDD is used)
    // C2:C0 = 0b010, VREF1:VREF0 = 0b00, PD1:PD0 = 0b00, G = 0b0
    byte configBytes[] { 0b01000000, (byte) (currentVoltage >> 4), (byte) ((currentVoltage << 4) & 0xF0) };
    Buffer configBuffer({configBytes, 3});
    queue(configBuffer);
    send();
}

DAC::~DAC() {
    // C2:C0 = 0b00x, PD1:PD0 = 0b11
    byte configBytes[] { (byte) (0b00110000 | ((MIN_VOLTAGE >> 4) & 0x0F)), (byte) MIN_VOLTAGE };
    Buffer configBuffer({configBytes, 2});
    queue(configBuffer);
    send();
}

void DAC::setVoltage(UVoltage voltage) {
    if (currentVoltage == voltage) {
        return;
    }
    //#ifdef DEBUG_ACTIVE
    //if (voltage > MAX_VOLTAGE || voltage < MIN_VOLTAGE) {
    //    std::cout << "Invalid voltage: " << voltage << "mV.\n";
    //}
    //#endif
    currentVoltage = voltage;
    // byte configBytes[] { (byte) ((currentVoltage >> 8) & 0x0F), (byte) currentVoltage };
    // Buffer configBuffer({configBytes, 3});

    byte configBytes[] { (byte) ((currentVoltage >> 4) & 0x0F), (byte) currentVoltage };
    Buffer configBuffer({ configBytes, 2 });
    queue(configBuffer);
    send();
}