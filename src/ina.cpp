#include "ina.hpp"

INA::INA(INA_Address addr) : I2C_Device((byte) addr, 5) {}


// pagina 16: 2.5µV / bit voor shuntspanning, tabel op pagina 17 bevestigd dit
Current INA::readCurrent() {
    // stel het MFR_READ_VSHUNT register in
    byte registerByte[] = { MFR_READ_VSHUNT };
    queue({ registerByte, 1 });
    send();
    // lees het register uit en interpreteer de ontvangen data
    sendAndAwait(2);
    const Buffer& received = readResponse();
    short receivedData = (received.data[1] << 8) | received.data[0];
    Voltage shuntMicroVoltage = receivedData * 2.5;
    return shuntMicroVoltage / rShunt;
}

// pagina 16: 2.5µV / bit voor shuntspanning, tabel op pagina 17 bevestigd dit
Voltage INA::readShuntVoltage() {
    // stel het MFR_READ_VSHUNT register in
    byte registerByte[] = { MFR_READ_VSHUNT };
    queue({ registerByte, 1 });
    send();
    // lees het register uit en interpreteer de ontvangen data
    sendAndAwait(2);
    const Buffer& received = readResponse();
    short receivedData = (received.data[1] << 8) | received.data[0];
    Voltage shuntMicroVoltage = receivedData * 2.5;
    return shuntMicroVoltage;
}

// pagina 16: 1.25mV / bit voor busspanning
UVoltage INA::readVoltage() {
    // stel het READ_VIN register in
    byte registerByte[] = { READ_VIN };
    queue({ registerByte, 1 });
    send();
    // lees het register uit en interpreteer de ontvangen data
    sendAndAwait(2);
    const Buffer& received = readResponse();
    short receivedData = (received.data[1] << 8) | received.data[0];
    return receivedData * 1.25;
}