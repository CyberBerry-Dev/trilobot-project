#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <cstdint>

// These constants are often missing from the header in C++
#define TRILO_SMBUS_WRITE      0
#define TRILO_SMBUS_BYTE_DATA  2
#define TRILO_I2C_SMBUS        0x0720

// Manually defining the data union since yours is "incomplete"
union i2c_smbus_data  {
    uint8_t byte;
    uint16_t word;
    uint8_t block[34];
};

int trilo_write_byte(int file, uint8_t reg, uint8_t value) {
    union i2c_smbus_data data;
    data.byte = value;

    struct i2c_smbus_ioctl_data args;
    args.read_write = TRILO_SMBUS_WRITE;
    args.command = reg;
    args.size = TRILO_SMBUS_BYTE_DATA;
    args.data = &data;
    //args.data = (union i2c_smbus_data*)&data; // Cast to the system type

    return ioctl(file, TRILO_I2C_SMBUS, &args);
}

int main() {
    int file = open("/dev/i2c-1", O_RDWR);
    if (file < 0) {
        std::cerr << "Open failed. Try: sudo ./trilo_led" << std::endl;
        return 1;
    }
    // I2C_SLAVE, 0x54 == LED chipset
    if (ioctl(file, I2C_SLAVE, 0x54) < 0) {
        std::cerr << "LED Driver 0x54 not found." << std::endl;
        return 1;
    }

    // 1. Wake up the chip
    trilo_write_byte(file, 0x00, 0x01);
    
    // 2. Enable all LED channels
    trilo_write_byte(file, 0x13, 0xFF);
    //trilo_write_byte(file, 0x14, 0xFF);

    // 3. Set LED 1 (Underlight Front Left) to medium brightness
    trilo_write_byte(file, 0x01, 0);
    trilo_write_byte(file, 0x02, 0);
    trilo_write_byte(file, 0x03, 0);
    //trilo_write_byte(file, 0x02, 2);

    // 4. Send the "Update" command to make it visible
    trilo_write_byte(file, 0x16, 0xFF);

    std::cout << "Success! The Trilobot should have a light on now." << std::endl;

    close(file);
    return 0;
}