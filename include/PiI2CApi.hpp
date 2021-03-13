#ifndef USING_RPI
// deze functies komen van wiringPiI2C.h API, zie http://wiringpi.com/reference/i2c-library/

// devId is het adres van het I²C-apparaat
int wiringPiI2CSetup(int devId);

// // lezen van apparaat fd
// int wiringPiI2CRead(int fd);

// // schrijven naar apparaat fd
int wiringPiI2CWrite(int, int data);

// deze functies komen van unistd.h, voor (via file handles) bepaalde operaties te doen

// __nbytes lezen uit __fd en opslaan in __buf
int read (int __fd, void *__buf, int __nbytes);

// __n bytes schrijven naar __fd, vanaf __buf
int write (int __fd, const void *__buf, int __n);

#else
#error "RPi is ingesteld, gelieve nergens wiringPiI2CApi te includen, en in dep laats wiringPiI2C.h te gebruiken."
#endif