#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdint.h>

#define DUTY_MIN -100
#define DUTY_MAX  100

// Initialize all motor control pins and PWM
void motor_init(void);

// Set absolute duty (-100 to 100)
void motor_a_set(int duty);
void motor_b_set(int duty);

// Increment or decrement duty
void motor_a_increment(void);
void motor_a_decrement(void);
void motor_b_increment(void);
void motor_b_decrement(void);

// Stop both motors
void motor_stop_all(void);

// Test
void motor_test_all(void);
void motor_turn_left(void);
void motor_turn_right(void);
void motor_spin_in_place(void);

#endif // MOTOR_CONTROL_H
