#include "i2c_device.hpp"
#ifdef USING_RPI
#include "pi_i2c.hpp"
#include <unistd.h>
#else
#include "pi_i2c_api.hpp"
#endif

I2C_Device::I2C_Device(byte address, byte bufferSize)
: address(address)
, fileHandle(setup(address))
, bytesToSend({new byte[bufferSize], bufferSize})
, currentSendBufferSize(0)
, bytesReceived({new byte[bufferSize], bufferSize})
, currentReceivedBufferSize(0) {}

I2C_Device::~I2C_Device() {
    delete[] bytesToSend.data;
    delete[] bytesReceived.data;
}

void I2C_Device::send() {
    write(fileHandle, bytesToSend.data, currentSendBufferSize);
    currentSendBufferSize = 0;
}

void I2C_Device::sendAndAwait(byte nBytes) {
    send();
    read(fileHandle, bytesReceived.data + currentReceivedBufferSize, nBytes);
    currentReceivedBufferSize += nBytes;
}

void I2C_Device::queue(const Buffer& b) {
    for (byte i = 0; i < b.size; ++i) {
        bytesToSend.data[currentSendBufferSize + i] = b.data[i];
    }
    currentSendBufferSize += b.size;
}

const Buffer& I2C_Device::readResponse() {
    currentReceivedBufferSize = 0;
    return bytesReceived;
}