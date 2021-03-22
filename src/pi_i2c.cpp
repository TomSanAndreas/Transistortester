#include "pi_i2c.hpp"
#ifdef USING_RPI
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>

#define I2C_SMBUS	0x0720	/* SMBus-level access */
#define I2C_SLAVE	0x0703
#define I2C_SMBUS_WRITE	    0
#define I2C_SMBUS_BYTE_DATA	2

struct i2c_smbus_ioctl_data {
  char read_write;
  unsigned char command;
  int size;
  union i2c_smbus_data *data;
};

int setup(const int devId) {
    int rev ;
    const char *device = "/dev/i2c-1"; // /dev/i2c-0 is also possible, it is based on the revision of the used board
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
    args.read_write = I2C_SMBUS_WRITE;
    args.command    = 0xFF;
    args.size       = I2C_SMBUS_BYTE_DATA;
    args.data       = nullptr;
    return ioctl(fd, I2C_SMBUS, &args) != -1;
}
#endif