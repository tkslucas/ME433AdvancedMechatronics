#include <stdio.h>
#include <stdlib.h>

#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include "motor_control.h"

#define AIN1 16
#define AIN2 17
#define BIN1 18
#define BIN2 19
#define LED_LEFT 28   // For motor A
#define LED_RIGHT 26  // For motor B

#define WRAP_VALUE 99
#define CLOCK_DIV 150.0f

static int duty_a = 0;
static int duty_b = 0;

static void init_pwm_pin(uint gpio);
static void set_motor_pwm(uint in1, uint in2, int duty);
static void set_led_pwm(uint led_gpio, int duty);

void motor_init(void) {
    init_pwm_pin(AIN1); init_pwm_pin(AIN2);
    init_pwm_pin(BIN1); init_pwm_pin(BIN2);
    init_pwm_pin(LED_LEFT);
    init_pwm_pin(LED_RIGHT);
}

static void init_pwm_pin(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice, CLOCK_DIV);
    pwm_set_wrap(slice, WRAP_VALUE);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), 0);
    pwm_set_enabled(slice, true);
}

static void set_motor_pwm(uint in1, uint in2, int duty) {
    duty = duty > 100 ? 100 : duty;
    duty = duty < -100 ? -100 : duty;

    uint slice = pwm_gpio_to_slice_num(in1);
    uint ch1 = pwm_gpio_to_channel(in1);
    uint ch2 = pwm_gpio_to_channel(in2);

    if (duty > 0) {
        pwm_set_chan_level(slice, ch1, duty * WRAP_VALUE / 100);
        pwm_set_chan_level(slice, ch2, 0);
    } else if (duty < 0) {
        pwm_set_chan_level(slice, ch1, 0);
        pwm_set_chan_level(slice, ch2, -duty * WRAP_VALUE / 100);
    } else {
        pwm_set_chan_level(slice, ch1, 0);
        pwm_set_chan_level(slice, ch2, 0);
    }
}

static void set_led_pwm(uint led_gpio, int duty) {
    uint slice = pwm_gpio_to_slice_num(led_gpio);
    uint channel = pwm_gpio_to_channel(led_gpio);

    int abs_duty = abs(duty);
    int brightness = 0;

    if (abs_duty >= 60) {
        int scaled = abs_duty - 60;           // Range: 0–40
        brightness = (scaled * scaled * WRAP_VALUE) / (40 * 40);  // Quadratic mapping
    }

    pwm_set_chan_level(slice, channel, brightness);
}

void motor_a_set(int duty) {
    duty_a = duty;
    set_motor_pwm(AIN2, AIN1, duty_a);  // AIN2/AIN1 swapped for reversed mount
    set_led_pwm(LED_LEFT, duty_a);
}

void motor_b_set(int duty) {
    duty_b = duty;
    set_motor_pwm(BIN1, BIN2, duty_b);
    set_led_pwm(LED_RIGHT, duty_b);
}

void motor_a_increment(void) {
    if (duty_a < 100) duty_a++;
    motor_a_set(duty_a);
}

void motor_a_decrement(void) {
    if (duty_a > -100) duty_a--;
    motor_a_set(duty_a);
}

void motor_b_increment(void) {
    if (duty_b < 100) duty_b++;
    motor_b_set(duty_b);
}

void motor_b_decrement(void) {
    if (duty_b > -100) duty_b--;
    motor_b_set(duty_b);
}

void motor_stop_all(void) {
    motor_a_set(0);
    motor_b_set(0);
}

void motor_test_all(void) {
    // 0 → 100
    for (int d = 0; d <= 100; d++) {
        motor_a_set(d);
        motor_b_set(d);
        printf("Ramp Up: Duty = %d\n", d);
        sleep_ms(50);
    }

    // 100 → -100
    for (int d = 100; d >= -100; d--) {
        motor_a_set(d);
        motor_b_set(d);
        printf("Reverse Sweep: Duty = %d\n", d);
        sleep_ms(50);
    }

    // -100 → 0
    for (int d = -100; d <= 0; d++) {
        motor_a_set(d);
        motor_b_set(d);
        printf("Ramp Down: Duty = %d\n", d);
        sleep_ms(50);
    }

    motor_stop_all();
    printf("Motors stopped. Pausing...\n");
    sleep_ms(100);
}

void motor_turn_left(void) {
    for (int d = 60; d <= 100; d++) {
        motor_a_set(d / 2);
        motor_b_set(d);
        printf("Turning Left: A = %d, B = %d\n", d / 2, d);
        sleep_ms(50);
    }
    motor_stop_all();
    sleep_ms(100);
}

void motor_turn_right(void) {
    for (int d = 60; d <= 100; d++) {
        motor_a_set(d);
        motor_b_set(d / 2);
        printf("Turning Right: A = %d, B = %d\n", d, d / 2);
        sleep_ms(50);
    }
    motor_stop_all();
    sleep_ms(100);
}

void motor_spin_in_place(void) {
    for (int d = 60; d <= 100; d++) {
        motor_a_set(d);
        motor_b_set(-d);
        printf("Spinning: A = %d, B = %d\n", d, -d);
        sleep_ms(50);
    }
    for (int d = 100; d >= 60; d--) {
        motor_a_set(d);
        motor_b_set(-d);
        printf("Spinning: A = %d, B = %d\n", d, -d);
        sleep_ms(50);
    }
    motor_stop_all();
    sleep_ms(100);
}