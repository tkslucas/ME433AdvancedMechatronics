/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN 15
#define BUTTON_GPIO_PIN 16

bool led_state = 0;
int times_pressed = 0;

// Toggle LED when button is pressed
void gpio_callback(uint gpio, uint32_t events) {
    gpio_set_irq_enabled(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, false);
    led_state = !led_state;
    times_pressed++;
    gpio_put(LED_PIN, led_state);
    printf("# times button was pressed = %d\n", times_pressed);
    sleep_ms(300); // for debouncing
    gpio_set_irq_enabled(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true);
}

void pico_gpio_init(void) {
    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, led_state);

    // Button
    gpio_init(BUTTON_GPIO_PIN);
    gpio_set_dir(BUTTON_GPIO_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}

int main() {
    stdio_init_all();
    pico_gpio_init();
    while (true) {
        sleep_ms(1000);
    }
}
