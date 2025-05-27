#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include <math.h>

// SPI Configuration
#define SPI_PORT        spi0
#define PIN_MISO        16
#define PIN_CS          17      // DAC chip select
#define PIN_SCK         18
#define PIN_MOSI        19
#define RAM_CS          13      // RAM chip select

// Function prototypes
void init_ram(void);
void ram_write(uint16_t address, float value);
float ram_read(uint16_t address);
void writeDAC(int channel, float voltage);

static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // Small delay for timing
    gpio_put(cs_pin, 0);                // Active low
    asm volatile("nop \n nop \n nop"); // Small delay for timing
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // Small delay for timing
    gpio_put(cs_pin, 1);                // Inactive high
    asm volatile("nop \n nop \n nop"); // Small delay for timing
}

union FloatInt {
    float f;
    uint32_t i;
};

void floating_point_calculations() {
    volatile float f1, f2;
    printf("Enter two floats to use:");
    scanf("%f %f", &f1, &f2);
    volatile float f_add, f_sub, f_mul, f_div;

    absolute_time_t t_add_0 = get_absolute_time();
    for (int count = 0; count < 1000; count++) {
        f_add = f1+f2;
    }
    absolute_time_t t_add_1 = get_absolute_time();
    uint64_t start_time_add = to_us_since_boot(t_add_0);
    uint64_t end_time_add   = to_us_since_boot(t_add_1);
    float add_time = ((float)(end_time_add - start_time_add) / 1000.0) / 0.006667;
    printf("\nTime for FP add: %.3f\n", add_time);

    absolute_time_t t_sub_0 = get_absolute_time();
    for (int count = 0; count < 1000; count++) {
        f_sub = f1-f2;
    }
    absolute_time_t t_sub_1 = get_absolute_time();
    uint64_t start_time_sub = to_us_since_boot(t_sub_0);
    uint64_t end_time_sub   = to_us_since_boot(t_sub_1);
    float sub_time = ((float)(end_time_sub - start_time_sub) / 1000.0) / 0.006667;
    printf("\nTime for FP sub: %.3f\n", sub_time);

    absolute_time_t t_mul_0 = get_absolute_time();
    for (int count = 0; count < 1000; count++) {
        f_mul = f1*f2;
    }
    absolute_time_t t_mul_1 = get_absolute_time();
    uint64_t start_time_mul = to_us_since_boot(t_mul_0);
    uint64_t end_time_mul   = to_us_since_boot(t_mul_1);
    float mul_time = ((float)(end_time_mul - start_time_mul) / 1000.0) / 0.006667;
    printf("\nTime for FP mul: %.3f\n", mul_time);

    absolute_time_t t_div_0 = get_absolute_time();
    for (int count = 0; count < 1000; count++) {
        f_div = f1/f2;
    }
    absolute_time_t t_div_1 = get_absolute_time();
    uint64_t start_time_div = to_us_since_boot(t_div_0);
    uint64_t end_time_div   = to_us_since_boot(t_div_1);
    float div_time = ((float)(end_time_div - start_time_div) / 1000.0) / 0.006667;
    printf("\nTime for FP div: %.3f\n", div_time);

    printf("\nResults: \n%f+%f=%f \n%f-%f=%f \n%f*%f=%f \n%f/%f=%f\n", f1,f2,f_add, f1,f2,f_sub, f1,f2,f_mul, f1,f2,f_div);
}

int main() {
    // Initialize stdio for USB communication
    stdio_init_all();

    // Initialize SPI at 1MHz
    spi_init(SPI_PORT, 1000 * 1000);
    
    // Configure GPIO functions
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(RAM_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Initialize chip select pins (active low)
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    gpio_set_dir(RAM_CS, GPIO_OUT);
    gpio_put(RAM_CS, 1);

    // Initialize RAM for sequential operation
    init_ram();

    // Wait for USB connection
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    // Generate and store sine wave in RAM
    uint16_t address = 0;
    float time = 0.0;
    for (int i = 0; i < 1000; i++) {
        // Generate sine wave centered at 1.65V (0-3.3V range)
        float voltage = 1.65 * sin(2.0 * M_PI * time) + 1.65;
        ram_write(address, voltage);
        time += 0.01;       // Increment time
        address += 4;       // Move to next 32-bit address
    }

    // Continuously read and output the stored waveform
    address = 0;
    while (true) {
        float voltage = ram_read(address);
        writeDAC(0, voltage);   // Output to DAC channel 0
        
        address += 4;           // Move to next sample
        if (address > 3996) {   // Wrap around at end of buffer (1000 samples * 4 bytes)
            address = 0;
        }
        sleep_ms(10);           // Control output rate
    }
}

void init_ram(void) {
    uint8_t config[2] = {
        0b00000001,     // Write status register command
        0b01000000      // Sequential mode configuration
    };
    
    cs_select(RAM_CS);
    spi_write_blocking(SPI_PORT, config, sizeof(config));
    cs_deselect(RAM_CS);
}

void ram_write(uint16_t a, float v) {
    uint8_t buffer[7];
    union FloatInt converter;
    converter.f = v;

    // Prepare write command and address
    buffer[0] = 0b00000010;     // Write command
    buffer[1] = a >> 8;         // Address high byte
    buffer[2] = a & 0xFF;       // Address low byte
    
    // Split float into 4 bytes (big-endian)
    buffer[3] = converter.i >> 24;
    buffer[4] = converter.i >> 16;
    buffer[5] = converter.i >> 8;
    buffer[6] = converter.i;

    cs_select(RAM_CS);
    spi_write_blocking(spi_default, buffer, sizeof(buffer));
    cs_deselect(RAM_CS);
}

float ram_read(uint16_t a) {
    uint8_t out_buffer[7] = {0};    // Output buffer (command + address)
    uint8_t in_buffer[7] = {0};     // Input buffer (will hold returned data)

    // Prepare read command and address
    out_buffer[0] = 0b00000011;     // Read command
    out_buffer[1] = a >> 8;         // Address high byte
    out_buffer[2] = a & 0xFF;       // Address low byte

    cs_select(RAM_CS);
    spi_write_read_blocking(spi_default, out_buffer, in_buffer, sizeof(out_buffer));
    cs_deselect(RAM_CS);

    // Reconstruct float from 4 bytes (big-endian)
    union FloatInt converter;
    converter.i = (in_buffer[3] << 24) | (in_buffer[4] << 16) | 
                  (in_buffer[5] << 8)  | in_buffer[6];
    
    return converter.f;
}

void writeDAC(int channel, float voltage) {
    uint8_t data[2];
    uint16_t dac_value = 0;

    // Prepare DAC command bits
    dac_value |= channel << 15;             // Channel select
    dac_value |= 0b111 << 12;               // Command bits (write and update)
    
    // Convert voltage to 10-bit DAC value (0-1023)
    uint16_t analog_value = (uint16_t)(voltage * 1023 / 3.3);
    dac_value |= analog_value << 2;         // Shift to proper position

    // Split 16-bit command into two bytes
    data[0] = dac_value >> 8;               // MSB first
    data[1] = dac_value & 0xFF;             // LSB second

    cs_select(PIN_CS);
    spi_write_blocking(spi_default, data, sizeof(data));
    cs_deselect(PIN_CS);
}