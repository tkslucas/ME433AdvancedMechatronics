#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define BUTTON_GPIO_PIN 16

// Called when button is pressed
void gpio_callback(uint gpio, uint32_t events) {
    gpio_set_irq_enabled(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, false);

    uint16_t result = adc_read();
    float voltage = result / 4095.0 * 3.3;
    printf("12-bit value = %d\n", result);
    printf("Voltage = %2fV\n", voltage);

    sleep_ms(300); // for debouncing
    gpio_set_irq_enabled(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true);
}

void pico_init_all(void) {
    // stdio
    stdio_init_all();

    // Button
    gpio_init(BUTTON_GPIO_PIN);
    gpio_set_dir(BUTTON_GPIO_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // ADC
    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0

    // CDC USB
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("CDC USB Start!\n");
}

int main() {
    pico_init_all();

    while (true) {
        sleep_ms(100);
    }
}