#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW        false
#define NUM_PIXELS     4
#define BRIGHTNESS     0.15f
#define SAT            1.0f
#define LED_OFFSET     90
#define PWM_PIN        16
#define PWM_WRAP       60000

#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN     PICO_DEFAULT_WS2812_PIN
#else
#define WS2812_PIN     10
#endif

#if WS2812_PIN >= NUM_BANK0_GPIOS
#error "WS2812 pin must be < 32 on this platform"
#endif

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} wsColor;

int loop_counter = 0;
wsColor leds[4];
uint32_t led_colors[4];
float hues[4];

PIO pio;
uint sm;
uint offset;

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb);
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
wsColor HSBtoRGB(float hue, float sat, float brightness);
void update_hues();
void update_leds();
void init_pwm();
uint16_t update_pwm();

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(5);
    }

    init_pwm();

    // Initialize WS2812 PIO program
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
        &ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Main control loop
    while (loop_counter < 361) {
        if (loop_counter == 360) {
            loop_counter = 0;
        }

        update_leds();
        pwm_set_gpio_level(PWM_PIN, update_pwm());

        loop_counter++;
        sleep_ms(13);  // Total ~14ms per iteration (including 1ms in update_leds)
    }

    // Clean up
    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

wsColor HSBtoRGB(float hue, float sat, float brightness) {
    float red = 0.0f, green = 0.0f, blue = 0.0f;

    if (sat == 0.0f) {
        red = green = blue = brightness;
    } else {
        hue = (hue == 360.0f) ? 0 : hue;
        int slice = hue / 60.0f;
        float hue_frac = (hue / 60.0f) - slice;

        float aa = brightness * (1.0f - sat);
        float bb = brightness * (1.0f - sat * hue_frac);
        float cc = brightness * (1.0f - sat * (1.0f - hue_frac));

        switch (slice) {
            case 0: red = brightness; green = cc;    blue = aa;    break;
            case 1: red = bb;        green = brightness; blue = aa;    break;
            case 2: red = aa;        green = brightness; blue = cc;    break;
            case 3: red = aa;        green = bb;     blue = brightness; break;
            case 4: red = cc;        green = aa;     blue = brightness; break;
            case 5: red = brightness; green = aa;     blue = bb;    break;
            default: break;
        }
    }

    wsColor c = {
        .r = (uint8_t)(red * 255.0f),
        .g = (uint8_t)(green * 255.0f),
        .b = (uint8_t)(blue * 255.0f)
    };
    return c;
}

void update_hues() {
    hues[0] = loop_counter;
    hues[1] = (loop_counter + LED_OFFSET) % 360;
    hues[2] = (loop_counter + 2 * LED_OFFSET) % 360;
    hues[3] = (loop_counter + 3 * LED_OFFSET) % 360;
}

void update_leds() {
    update_hues();

    for (int i = 0; i < 4; i++) {
        leds[i] = HSBtoRGB(hues[i], SAT, BRIGHTNESS);
        led_colors[i] = urgb_u32(leds[i].r, leds[i].g, leds[i].b);
        put_pixel(pio, sm, led_colors[i]);
    }
    sleep_ms(1);  // Ensure reset
}

void init_pwm() {
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    pwm_set_clkdiv(slice_num, 50);
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_enabled(slice_num, true);
    pwm_set_gpio_level(PWM_PIN, 0);
}

uint16_t update_pwm() {
    float fraction;
    if (loop_counter <= 180) {
        fraction = (loop_counter / 180.0f) * 0.1f + 0.025f;  // Forward sweep
    } else {
        fraction = -(loop_counter % 180 / 180.0f) * 0.1f + 0.125f;  // Backward sweep
    }
    return (uint16_t)(fraction * PWM_WRAP);
}