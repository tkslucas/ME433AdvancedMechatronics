#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include "cam.h"
#include "motor_control.h"

int main()
{
    stdio_init_all();
    motor_init();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    sleep_ms(50);

    printf("Time to follow the line!\n");

    init_camera_pins();
    sleep_ms(200);
 
    while (true) {
        motor_test_all();
        motor_turn_left();
        motor_turn_right();
        motor_spin_in_place();       
        
        // uncomment these and printImage() when testing with python 
        //char m[10];
        //scanf("%s",m);

        /*// Camera
        setSaveImage(1);
        while(getSaveImage()==1){}
        convertImage();
        int com = findLine(IMAGESIZEY/2); // calculate the position of the center of the ine
        setPixel(IMAGESIZEY/2,com,0,255,0); // draw the center so you can see it in python
        //printImage();
        printf("%d\r\n",com); // comment this when testing with python
        */
    }
}

