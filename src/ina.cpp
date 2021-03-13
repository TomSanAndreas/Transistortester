#include "ina.hpp"

INA::INA(INA_Address addr) : I2C_Device((byte) addr, 5) {
    // set CAL register, so current can be read directly
    // TODO
}


// page 16: 2.5µV / bit for shunt voltage, table at page 17 confirms this
Current INA::readCurrent() {
    // TODO implement this using READ_IIN / READ_IOUT
    // now implemented using MFR_READ_VSHUNT
    // set the MFR_READ_VSHUNT register
    byte registerByte[] = { MFR_READ_VSHUNT };
    queue({ registerByte, 1 });
    send();
    // now read from MFR_READ_VSHUNT register
    sendAndAwait(2);
    const Buffer& received = readResponse();
    // FIXME indices might be inverted, description and diagram aren't consistent on page 20
    short receivedData = (received.data[1] << 8) | received.data[0];
    Voltage shuntMicroVoltage = receivedData * 2.5;
    return shuntMicroVoltage / R_SHUNT;
}

// page 16: 1.25mV / bit for bus voltage
UVoltage INA::readVoltage() {
    // request data using READ_VIN
    byte registerByte[] = { READ_VIN };
    queue({ registerByte, 1 });
    send();
    // now read from READ_VIN register
    sendAndAwait(2);
    const Buffer& received = readResponse();
    // FIXME indices might be inverted, description and diagram aren't consistent on page 20
    short receivedData = (received.data[1] << 8) | received.data[0];
    return receivedData * 1.25;
}