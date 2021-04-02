#pragma once
typedef unsigned char byte;

#include "parameters.hpp"

typedef unsigned short UVoltage; // is always between 0 and 65535, max voltage is 4960 for a DAC
                                 // because of the limitation, only the lower 12 bits are used
typedef short Voltage;           // signed voltage, mostly used during calculations with the INA

typedef float Current;           // can be positive or negative, so this is signed

class I2C_Device;

struct Buffer {
    byte* data;
    byte size;
};

class I2C_Device {
    protected:
        // device address
        byte address;
        // file handle, used for read() and write() functions
        int fileHandle;
        // send the data from the buffer
        void send();
        // send the data from the buffer and wait for response
        void sendAndAwait(byte nBytes);

        void queue(const Buffer&);
        const Buffer& readResponse();
    private:

        Buffer bytesToSend;
        // between 0 and bytesToSend.size, so no overflow occurs
        byte currentSendBufferSize;

        Buffer bytesReceived;
        // between 0 and bytesReceived.size, so new data would not overlap with older data
        byte currentReceivedBufferSize;
    public:
        I2C_Device(byte address, byte bufferSize = 0x0F);
        ~I2C_Device();
};
