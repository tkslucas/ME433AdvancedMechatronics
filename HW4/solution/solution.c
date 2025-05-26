#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include <math.h>

// GPIO
#define LED_PIN 15
#define BUTTON_GPIO_PIN 16

// SPI
#define PIN_CS PICO_DEFAULT_SPI_CSN_PIN
#define SPI_PORT spi0

volatile bool button_pressed = false;

// Called when button is pressed
void gpio_callback(uint gpio, uint32_t events) {
    gpio_set_irq_enabled(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, false);

    button_pressed = true;

    sleep_ms(300); // for debouncing
    gpio_set_irq_enabled(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true);
}

void pico_init_all(void) {
    // stdio
    stdio_init_all();

    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, false);

    // Button
    gpio_init(BUTTON_GPIO_PIN);
    gpio_set_dir(BUTTON_GPIO_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // ADC
    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0

    // CDC USB, waits until USB port has been opened
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("CDC USB Start!\n");

    // SPI
    spi_init(spi_default, 1000 * 1000); // the baud, or bits per second
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
}

#ifdef PICO_DEFAULT_SPI_CSN_PIN
static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}
#endif

void writeDAC(int channel, float voltage) {
    uint8_t data[2];
    int len = 2;

    uint16_t desired = 0;
    desired |= channel << 15;
    desired |= 0b111 << 12; 
    uint16_t v_conversion = (uint16_t)(voltage * 1023 / 3.3);
    desired |= v_conversion << 2;

    data[0] = (desired >> 8) & 0xFF; // MSB
    data[1] = desired & 0xFF; // LSB

    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, len); // send data to chip
    cs_deselect(PIN_CS);
}

int main() {
    pico_init_all();

    // Turns on LED
    gpio_put(LED_PIN, true);
    // Wait until a button is pressed
    printf("Waiting for button to be pressed...\n");
    while (!button_pressed) { sleep_ms(100); }
    printf("Button pressed!\n");
    // Turns off LED
    gpio_put(LED_PIN, false);

    while (true) {
        int count = 0;
        float t = 0.0;
        bool up = 1;

        float max_voltage = 3.30;
        float tri_wave = 0;
        float inc = 1.65 * 2.0 / 50.0;

        for(count = 0; count < 10000; count++){
            t = t + 0.01;
            float sine_wave = 1.65 * sin(4.0 * M_PI * t) + 1.65; // 2Hz sine
            writeDAC(0, sine_wave);

            // Triangle waveform
            if (up){
                tri_wave = tri_wave + inc;
                if (tri_wave > max_voltage){
                    tri_wave = max_voltage;
                    up = 0;
                }
            }
            else {
                tri_wave = tri_wave - inc;
                if (tri_wave < 0){
                    tri_wave = 0;
                    up = 1;
                }
            }
            writeDAC(1, tri_wave);
            sleep_ms(10); 
        }
    }
}