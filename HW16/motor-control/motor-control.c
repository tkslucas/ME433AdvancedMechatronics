#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define AIN1 16
#define AIN2 17
#define BIN1 18
#define BIN2 19

#define WRAP_VALUE 99
#define CLOCK_DIV 150.0f
#define DUTY_MAX 100
#define DUTY_MIN -100

static int duty_a = 0;
static int duty_b = 0;

void init_pwm_pin(uint gpio);
void set_motor_pwm(uint in1, uint in2, int duty);
void update_motor(char wheel, char cmd);

int main(void) {
    stdio_init_all();

    init_pwm_pin(AIN1); init_pwm_pin(AIN2);
    init_pwm_pin(BIN1); init_pwm_pin(BIN2);

    while (!stdio_usb_connected());
    sleep_ms(50);
    printf("DRV8833 Motor Control Ready\n");

    while (true) {
        char wheel = 0, cmd = 0;

        printf("Select motor: 'L' or 'R'\n");
        scanf(" %c", &wheel);
        printf("Command: '+' to speed up, '-' to slow down\n");
        scanf(" %c", &cmd);

        update_motor(wheel, cmd);
        printf("Duty A: %d | Duty B: %d\n", duty_a, duty_b);
        sleep_ms(50);
    }
}

void init_pwm_pin(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice, CLOCK_DIV);
    pwm_set_wrap(slice, WRAP_VALUE);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(gpio), 0);
    pwm_set_enabled(slice, true);
}

void set_motor_pwm(uint in1, uint in2, int duty) {
    uint slice1 = pwm_gpio_to_slice_num(in1);
    uint chan1 = pwm_gpio_to_channel(in1);
    uint chan2 = pwm_gpio_to_channel(in2);

    if (duty > 0) {
        pwm_set_chan_level(slice1, chan1, duty * WRAP_VALUE / 100);
        pwm_set_chan_level(slice1, chan2, 0);
    } else if (duty < 0) {
        pwm_set_chan_level(slice1, chan1, 0);
        pwm_set_chan_level(slice1, chan2, (-duty) * WRAP_VALUE / 100);
    } else {
        pwm_set_chan_level(slice1, chan1, 0);
        pwm_set_chan_level(slice1, chan2, 0);
    }
}

void update_motor(char wheel, char cmd) {
    int* duty = NULL;
    uint in1, in2;

    if (wheel == 'L' || wheel == 'l') {
        duty = &duty_a; in1 = AIN1; in2 = AIN2;
    } else if (wheel == 'R' || wheel == 'r') {
        duty = &duty_b; in1 = BIN1; in2 = BIN2;
    } else {
        printf("Invalid motor selection\n");
        return;
    }

    if (cmd == '+' && *duty < DUTY_MAX) (*duty)++;
    else if (cmd == '-' && *duty > DUTY_MIN) (*duty)--;
    else if (cmd != '+' && cmd != '-') {
        printf("Invalid command\n");
        return;
    }

    set_motor_pwm(in1, in2, *duty);
}
