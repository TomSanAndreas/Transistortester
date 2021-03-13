#include "i2c_device.hpp"
#ifdef USING_RPI
#include <wiringPiI2C.h>
#include <unistd.h>
#else
#include "PiI2CApi.hpp"
#endif

I2C_Device::I2C_Device(byte address, byte bufferSize)
: address(address)
, fileHandle(wiringPiI2CSetup(address))
, bytesToSend({new byte[bufferSize], bufferSize})
, currentSendBufferSize(0)
, bytesReceived({new byte[bufferSize], bufferSize})
, currentReceivedBufferSize(0) {
    #ifdef DEBUG_ACTIVE
    if (wiringPiI2CWrite(fileHandle, 0x00) == -1) {
        debugStatus |= (uint64_t) Status::NoResponse | (uint64_t) Status::Error;
        printStatus();
    } else {
        std::cout << "Apparaat 0x" << std::hex << address << " succesvol geinitialiseerd!\n";
    }
    #endif
}

I2C_Device::~I2C_Device() {
    delete[] bytesToSend.data;
    delete[] bytesReceived.data;
}

Status I2C_Device::send() {
    write(fileHandle, bytesToSend.data, currentSendBufferSize);
    currentSendBufferSize = 0;
    return Status::Success;
}

Status I2C_Device::sendAndAwait(byte nBytes) {
    Status s = send();
    read(fileHandle, bytesReceived.data + currentReceivedBufferSize, nBytes);
    currentReceivedBufferSize += nBytes;
    return Status::Success;
}

Status I2C_Device::queue(const Buffer& b) {
    if (b.size + currentSendBufferSize > bytesToSend.size) {
        #ifdef DEBUG_ACTIVE
        debugStatus |= (uint64_t) Status::BufferOverflow;
        #endif
        return Status::BufferOverflow;
    }
    for (byte i = 0; i < b.size; ++i) {
        bytesToSend.data[currentSendBufferSize + i] = b.data[i];
    }
    currentSendBufferSize += b.size;
    return Status::Success;
}

const Buffer& I2C_Device::readResponse() {
    currentReceivedBufferSize = 0;
    return bytesReceived;
}

#ifdef DEBUG_ACTIVE
void I2C_Device::printStatus() {
    if (debugStatus & (uint64_t) Status::Error) {
        std::cout << "An other error has occurred with device 0x" << std::hex << address << "\n";
    }
    if (debugStatus & (uint64_t) Status::BufferOverflow) {
        std::cout << "A buffer overflow has occurred with device 0x" << std::hex << address << "\n";
    }
    if (debugStatus & (uint64_t) Status::NoResponse) {
        std::cout << "Device 0x" << std::hex << address << " has not responded.\n";
    }
    // reset status
    debugStatus = 0;
}
#endif