#pragma once
typedef unsigned char byte;

#define DEBUG_ACTIVE 1
// #define USING_RPI 1

#ifdef DEBUG_ACTIVE
#include <iostream>
#endif

typedef unsigned short UVoltage; // is always between 0 and 65535, max voltage is 4960 for a DAC
                                 // because of the limitation, only the lower 12 bits are used
typedef short Voltage;           // signed voltage, mostly used during calculations with the INA

typedef short Current;           // can be positive or negative, so this is signed

class I2C_Device;

struct Buffer {
    byte* data;
    byte size;
};

enum class Status {
    Success = 0b0, // ACK received and no other errors occured
    Error = 0b1, // other error occurs
    BufferOverflow = 0b10, // currentXBufferSize > max size (send / receive buffer)
    NoResponse = 0b100 // ACK not received
};

class I2C_Device {
    private:
        #ifdef DEBUG_ACTIVE
        uint64_t debugStatus = 0;
        #endif

        Buffer bytesToSend;
        // between 0 and bytesToSend.size, so no overflow occurs
        byte currentSendBufferSize;

        Buffer bytesReceived;
        // between 0 and bytesReceived.size, so new data would not overlap with older data
        byte currentReceivedBufferSize;
    protected:
        // device address
        byte address;
        // file handle, used for read() and write() functions
        int fileHandle;
        // send the data from the buffer
        Status send();
        // send the data from the buffer and wait for response
        Status sendAndAwait(byte nBytes);

        Status queue(const Buffer&);
        const Buffer& readResponse();
    public:
        I2C_Device(byte address, byte bufferSize = 0x0F);
        ~I2C_Device();

        #ifdef DEBUG_ACTIVE
        void printStatus();
        #endif
};