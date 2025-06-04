#include <stdio.h>

#include "motor_control.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#define AIN1 17
#define AIN2 16
#define BIN1 18
#define BIN2 19

#define WRAP_VALUE 99
#define CLOCK_DIV 150.0f

static int duty_a = 0;
static int duty_b = 0;

static void init_pwm_pin(uint gpio);
static void set_motor_pwm(uint in1, uint in2, int duty);

void motor_init(void) {
    init_pwm_pin(AIN1); init_pwm_pin(AIN2);
    init_pwm_pin(BIN1); init_pwm_pin(BIN2);
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
    duty = duty > DUTY_MAX ? DUTY_MAX : duty;
    duty = duty < DUTY_MIN ? DUTY_MIN : duty;

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

void motor_a_set(int duty) {
    duty_a = duty;
    set_motor_pwm(AIN1, AIN2, duty_a);
}

void motor_b_set(int duty) {
    duty_b = duty;
    set_motor_pwm(BIN1, BIN2, duty_b);
}

void motor_a_increment(void) {
    if (duty_a < DUTY_MAX) duty_a++;
    set_motor_pwm(AIN1, AIN2, duty_a);
}

void motor_a_decrement(void) {
    if (duty_a > DUTY_MIN) duty_a--;
    set_motor_pwm(AIN1, AIN2, duty_a);
}

void motor_b_increment(void) {
    if (duty_b < DUTY_MAX) duty_b++;
    set_motor_pwm(BIN1, BIN2, duty_b);
}

void motor_b_decrement(void) {
    if (duty_b > DUTY_MIN) duty_b--;
    set_motor_pwm(BIN1, BIN2, duty_b);
}

void motor_stop_all(void) {
    duty_a = 0;
    duty_b = 0;
    set_motor_pwm(AIN1, AIN2, 0);
    set_motor_pwm(BIN1, BIN2, 0);
}

void motor_test_all(void) {
    // 0 → 100
    for (int d = 0; d <= 100; d++) {
        motor_a_set(d);
        motor_b_set(d);
        printf("Ramp Up: Duty = %d\n", d);
        sleep_ms(20);
    }

    // 100 → -100
    for (int d = 100; d >= -100; d--) {
        motor_a_set(d);
        motor_b_set(d);
        printf("Reverse Sweep: Duty = %d\n", d);
        sleep_ms(20);
    }

    // -100 → 0
    for (int d = -100; d <= 0; d++) {
        motor_a_set(d);
        motor_b_set(d);
        printf("Ramp Down: Duty = %d\n", d);
        sleep_ms(20);
    }

    motor_stop_all();
    printf("Motors stopped. Pausing...\n");
    sleep_ms(1000);
}