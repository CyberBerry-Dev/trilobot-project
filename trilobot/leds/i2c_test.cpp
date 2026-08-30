#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

int main() {
    const char *device = "/dev/i2c-1";
    int file = open(device, O_RDWR);

    if (file < 0) {
        std::cerr << "FAILED: Could not open /dev/i2c-1. Is I2C enabled?" << std::endl;
        return 1;
    }

    // Try to talk to the IO Expander (0x18)
    if (ioctl(file, I2C_SLAVE, 0x18) < 0) {
        std::cerr << "FAILED: Could not find Trilobot at 0x18." << std::endl;
    } else {
        std::cout << "SUCCESS: I2C bus open and device 0x18 found!" << std::endl;
    }

    close(file);
    return 0;
}
