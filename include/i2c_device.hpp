#pragma once
typedef unsigned char byte;

#include "parameters.hpp"

typedef unsigned short UVoltage; // is altijd tussen 0 en 65535, max voltage is 4960 per DAC
                                 // vanwege deze limitatie worden enkel de laagste 12 bits gebruikt
typedef short Voltage;           // signed voltage, wordt meestal voor interne berekeningen met de INA gebruikt

typedef float Current;           // kommagetal, wordt bekomen door een deling

class I2C_Device;

struct Buffer {
    byte* data;
    byte size;
};

class I2C_Device {
    protected:
        // adres van het apparaat
        byte address;
        // file handle, gebruikt samen read() en write() functies
        int fileHandle;
        // zend de data in de buffer
        void send();
        // zend de data in de buffer en wacht voor een antwoord
        void sendAndAwait(byte nBytes);

        void queue(const Buffer&);
        const Buffer& readResponse();
    private:

        Buffer bytesToSend;
        // tussen 0 en bytesToSend.size, zodat er geen overflow plaatsvindt
        byte currentSendBufferSize;

        Buffer bytesReceived;
        // tussen 0 en bytesReceived.size, zodat nieuwe data niet zou overlappen met oude data
        byte currentReceivedBufferSize;
    public:
        I2C_Device(byte address, byte bufferSize = 0x0F);
        ~I2C_Device();
};
