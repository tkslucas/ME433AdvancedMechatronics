#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define LED_PIN 27

int main() {
    stdio_init_all();
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

    uint slice = pwm_gpio_to_slice_num(LED_PIN);
    uint channel = pwm_gpio_to_channel(LED_PIN);

    pwm_set_wrap(slice, 99);           // Set PWM resolution
    pwm_set_clkdiv(slice, 150.0f);     // Slow it down so brightness is visible
    pwm_set_chan_level(slice, channel, 0); // Start off
    pwm_set_enabled(slice, true);

    while (true) {
        for (int i = 0; i <= 100; i++) {
            pwm_set_chan_level(slice, channel, i); // Fade up
            sleep_ms(10);
        }
        for (int i = 100; i >= 0; i--) {
            pwm_set_chan_level(slice, channel, i); // Fade down
            sleep_ms(10);
        }
    }
}
