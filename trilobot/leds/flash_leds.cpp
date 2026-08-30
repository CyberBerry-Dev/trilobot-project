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
    args.data = reinterpret_cast<union i2c_smbus_data*>(&data);
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

    // SETUP
    // 1 - wake the LED chipset
    trilo_write_byte(file, 0x00, 0x01);
    // 2 - enable all LED channels for all LEDs
    trilo_write_byte(file, 0x13, 0xFF);
    trilo_write_byte(file, 0x14, 0xFF);
    trilo_write_byte(file, 0x15, 0xFF);
    
    // 3 - loop through 6 of the LEDs ( 2 but 3 colours each RGB )
    while (true) {
        for (uint8_t led = 0x01; led <= 0x12; led++) {

            // turn on the LED
            trilo_write_byte(file, led, 200);
            trilo_write_byte(file, 0x16, 0xFF);
            
            sleep(2); // wait 200ms
            
            // turn off the LED
            trilo_write_byte(file, led, 0);
            trilo_write_byte(file, 0x16, 0xFF);            
        }

    }
    


    std::cout << "Success! The Trilobot should show a small light disco now. Ctrl + C to exit" << std::endl;
    std::cout << "C to exit" << std::endl;

    close(file);
    return 0;
}