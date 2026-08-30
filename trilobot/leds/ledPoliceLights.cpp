#include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <cstdint>
#include <csignal>   // Required for signal handling
#include <atomic>    // Required for thread-safe flag


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

// Create a flag that can be safely modified by the OS signal handler
std::atomic<bool> keep_running(true);

// This function triggers automatically when you press Ctrl + C
void signal_handler(int signum) {
    std::cout << "\nStopping safely... Shutting down all LEDs." << std::endl;
    keep_running = false;
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

    // Define the LED addresses for blue and red LEDs
    const uint8_t blue_leds[] = {0x06, 0x09, 0x0C};
    const uint8_t red_leds[]  = {0x01, 0x10, 0x0D};

    std::signal(SIGINT, signal_handler); // Register the signal handler for Ctrl + C

    std::cout << "Success! The Trilobot should enable police lights. Ctrl + C to exit" << std::endl;

    while (keep_running) {
        for (uint8_t led = 0x01; led <= 0x12; led++) {

            if (!keep_running) break;

            // Force all red side LEDs off in cache
            for (uint8_t led : red_leds) {
                trilo_write_byte(file, led, 0);
            }
            // Set all right-side blue LEDs to bright
            for (uint8_t led : blue_leds) {
                trilo_write_byte(file, led, 200);
            }
            // Commit the change to the hardware instantly
            trilo_write_byte(file, 0x16, 0xFF);
            
            // Pause for 200ms while Blue is lit up
            usleep(200000); 

            // Do the reverse for Red side LEDs
            for (uint8_t led : blue_leds) {
                trilo_write_byte(file, led, 0);
            }

            for (uint8_t led : red_leds) {
                trilo_write_byte(file, led, 200);
            }

            trilo_write_byte(file, 0x16, 0xFF);
            

            usleep(200000); 
        }
    }
    // Loop through all possible LEDs and force them off
    std::cout << "Cleaning up LED registry..." << std::endl;
    for (uint8_t led = 0x01; led <= 0x12; led++) {
        trilo_write_byte(file, led, 0);
    }

    trilo_write_byte(file, 0x16, 0xFF);

    close(file);
    std::cout << "All LEDs should now be off. Exiting." << std::endl;
    return 0;
}