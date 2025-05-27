#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C
#define SCL_PIN 5
#define SDA_PIN 4
#define I2C_PORT i2c0

// IO Expander Addresses
#define IO_EXPANDER_DEVICE_ADDRESS 0b0100000
#define IO_EXPANDER_IODIR_ADDRESS 0x00
#define IO_EXPANDER_IOCON_ADDRESS 0x05
#define IO_EXPANDER_GPIO_ADDRESS 0x09
#define IO_EXPANDER_OLAT_ADDRESS 0x0A

static uint8_t buf;     // Buffer to read data
uint8_t I2C_array[2];   // Buffer to send data (reg, data)

static void pico_init_all();
static void io_expander_init();
static void WritePin(uint8_t advice, uint8_t register, uint8_t data);
static uint8_t ReadPin(uint8_t advice, uint8_t register);

static void pico_set_led(bool led_state){
    gpio_put(PICO_DEFAULT_LED_PIN, led_state);
}

int main() {
    pico_init_all();

    uint8_t previous_voltage = 0b1;
    while (true) {
        buf = ReadPin(IO_EXPANDER_DEVICE_ADDRESS, IO_EXPANDER_GPIO_ADDRESS);
        uint8_t current_voltage = buf & 0b1;
        if (current_voltage != previous_voltage){
            sleep_ms(10); // debounce
            uint8_t confirm = ReadPin(IO_EXPANDER_DEVICE_ADDRESS, IO_EXPANDER_GPIO_ADDRESS) & 0b1;
            if (confirm == current_voltage){
                WritePin(IO_EXPANDER_DEVICE_ADDRESS, IO_EXPANDER_OLAT_ADDRESS, (previous_voltage << 7));
                previous_voltage = current_voltage;
            }
        }

        printf("GPIO 0: %d\r\n",current_voltage);
        pico_set_led(!gpio_get(PICO_DEFAULT_LED_PIN)); // Heartbeating led for debugging
        sleep_ms(100);
    }
}

static void pico_init_all() {
    // stdio
    stdio_init_all();

    // CDC USB, waits until USB port has been opened
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("CDC USB Start!\n");

    // Pico LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // I2C
    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);

    // IO Expander
    io_expander_init();
}

static void io_expander_init(){
    // Bite read mode
    WritePin(IO_EXPANDER_DEVICE_ADDRESS, IO_EXPANDER_IOCON_ADDRESS, 0b00100000);
    // GPIO 7 as output pin
    WritePin(IO_EXPANDER_DEVICE_ADDRESS, IO_EXPANDER_IODIR_ADDRESS, 0b01111111);
    // GPIO 7 to be default low
    WritePin(IO_EXPANDER_DEVICE_ADDRESS, IO_EXPANDER_OLAT_ADDRESS, 0b00000000);
}

static void WritePin(uint8_t device_addr, uint8_t reg_addr, uint8_t data){
    uint8_t I2C_buf[2];
    I2C_buf[0] = reg_addr;
    I2C_buf[1] = data;

    int result = i2c_write_blocking(I2C_PORT, device_addr, I2C_buf, 2, false);
    if (result < 0){
        printf("Error occurs druing Writing Communication!\r\n ");
    }
}

static uint8_t ReadPin(uint8_t device_addr, uint8_t reg_addr){
    uint8_t I2C_buf;
    int result;

    result = i2c_write_blocking(I2C_PORT, device_addr, &reg_addr, 1, true);
    if (result < 0){
        printf("Error occurs druing Writing Communication!\r\n ");
    }

    result = i2c_read_blocking(I2C_PORT, device_addr, &I2C_buf, 1, false);
    if (result < 0){
        printf("Error occurs druing Reading Communication!\r\n ");
    }

    return I2C_buf;
}
