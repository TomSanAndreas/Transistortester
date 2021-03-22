#include "pi_i2c.hpp"
#ifdef USING_RPI
#include <fcntl.h>
#include <asm/ioctl.h>

int setup(const int devId) {
    int rev ;
    const char *device = "/dev/i2c-0"; // also /dev/i2c-1 possible
    int fd;
    if ((fd = open(device, O_RDWR)) < 0) {
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE, devId) < 0) {
        return -1;
    }
    return fd;
}

bool hasResponded(int fd) {
    struct i2c_smbus_ioctl_data args;
    args.read_write = rw;
    args.command    = I2C_SMBUS_WRITE;
    args.size       = I2C_SMBUS_BYTE_DATA;
    args.data       = 0xFF;
    return ioctl(fd, I2C_SMBUS, &args) != -1;
}
#endif