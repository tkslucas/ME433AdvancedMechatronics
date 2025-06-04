#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "cam.h"
#include "motor_control.h"

const int MAX_DUTY = 100;
const int MIN_DUTY = 60;
const int BASE_DUTY = 70;
const int DEADBAND = 5;
const float GAIN = 0.3f;

int main() {
    stdio_init_all();
    motor_init();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    sleep_ms(50);

    printf("Time to follow the line!\n");

    init_camera_pins();
    sleep_ms(100);
 
    while (true) {
        setSaveImage(1);
        while(getSaveImage()==1) {}
        convertImage();

        int com = findLine(IMAGESIZEY/2);
        setPixel(IMAGESIZEY/2, com, 0, 255, 0);
        printf("%d\r\n", com);

        int image_center = IMAGESIZEX / 2;
        int error = com - image_center;

        if (abs(error) < DEADBAND) {
            motor_a_set(BASE_DUTY);
            motor_b_set(BASE_DUTY);
        } else {
            int adjust = (int)(GAIN * error);

            int left_duty  = BASE_DUTY + adjust;
            int right_duty = BASE_DUTY - adjust;

            if (left_duty > MAX_DUTY) left_duty = MAX_DUTY;
            if (left_duty < MIN_DUTY) left_duty = MIN_DUTY;
            if (right_duty > MAX_DUTY) right_duty = MAX_DUTY;
            if (right_duty < MIN_DUTY) right_duty = MIN_DUTY;

            motor_a_set(left_duty);
            motor_b_set(right_duty);
        }

        sleep_ms(10);
    }
}

