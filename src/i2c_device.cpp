#include "i2c_device.hpp"

I2C_Device::I2C_Device(byte address, byte bufferSize)
: address(address)
, bytesToSend({new byte[bufferSize], bufferSize})
, currentSendBufferSize(0)
, bytesReceived({new byte[bufferSize], bufferSize})
, currentReceivedBufferSize(0) {}

I2C_Device::~I2C_Device() {
    delete[] bytesToSend.data;
    delete[] bytesReceived.data;
}

Status I2C_Device::send() {
    // send address
    //TODO
    // send all the data within the sendBuffer
    for (byte i = 0; i < currentSendBufferSize; ++i) {
        //TODO
    }
    currentSendBufferSize = 0;
    //TODO in case of no ACK, set status
    //if (...)
    #ifdef DEBUG_ACTIVE
    debugStatus |= (uint64_t) Status::NoResponse;
    #endif
    return Status::NoResponse;
}

Status I2C_Device::sendAndAwait() {
    Status s = send();
    if (s == Status::NoResponse) {
        return Status::NoResponse;
    }
    // read all the data and fill up the bytesReceived buffer
    // ...
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

const Buffer& I2C_Device::read() {
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