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

int main() {
    pico_init_all();

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