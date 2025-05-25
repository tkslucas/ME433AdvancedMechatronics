#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define LED_PIN 15
#define BUTTON_GPIO_PIN 16

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
        // Ask user for number
        uint16_t number_of_samples = 0;
        printf("Enter number of analog samples to take (between 1 and 100): ");
        scanf("%d", &number_of_samples);

        // Reads the voltage on ADC0 that number of times at 100Hz
        for(int count_samples = 1; count_samples <= number_of_samples; count_samples++) { 
            uint16_t result = adc_read();

            float voltage = result / 4095.0 * 3.3;
            //printf("12-bit value = %d\n", result);
            printf("Voltage Sample #%2d = %2fV\n", count_samples, voltage);

            sleep_ms(10);
        }
    }
}