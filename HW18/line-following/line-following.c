#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "cam.h"
#include "motor_control.h"

const int MAX_DUTY = 100;
const int MIN_DUTY = 60;
const int BASE_DUTY = 65;
const int DEADBAND = 5;
const float GAIN = 0.9f;
const int LEFT_BIAS = 2;
const float LEFT_BIAS_SLOPE = 1.2;

int main() {
    stdio_init_all();
    motor_init();

    // while (!stdio_usb_connected()) {
    //     sleep_ms(100);
    // }

    //printf("Time to follow the line!\n");
    init_camera_pins();
    sleep_ms(20);
 
    while (true) {
        setSaveImage(1);
        while(getSaveImage()==1) {}
        convertImage();

        int com = findLineColumn(IMAGESIZEX/2);
        setPixel(IMAGESIZEX/2, com, 0, 255, 0);

        int image_center = IMAGESIZEX / 2;
        int error = com - image_center;
        printf("%d\r\n", error);

        if (abs(error) < DEADBAND) {
            motor_a_set(BASE_DUTY);
            motor_b_set(BASE_DUTY);
        } else {
            int adjust = (int)(GAIN * error);

            int left_duty  = BASE_DUTY + adjust + LEFT_BIAS;
            int right_duty = BASE_DUTY - adjust;

            if (left_duty > MAX_DUTY) left_duty = MAX_DUTY;
            if (left_duty < MIN_DUTY) left_duty = MIN_DUTY;
            if (right_duty > MAX_DUTY) right_duty = MAX_DUTY;
            if (right_duty < MIN_DUTY) right_duty = MIN_DUTY;

            motor_a_set(left_duty);
            motor_b_set(right_duty);
        }
    }
}

