# CDC, IO, and ADC

Instead of building from an example, try using the Raspberry Pi Pico extension in VSCode to make a new C/C++ project. Create the project name and select your repo to generate the template, but don't select any of the checkboxes.

## Where is the documentation?
Raspberry Pi provides documentation for using the SDK at [https://www.raspberrypi.com/documentation/pico-sdk/hardware.html](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html).  
There are also some higher level functions, like knowing time parameters, at [https://www.raspberrypi.com/documentation/pico-sdk/high_level.html](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html).

## CDC Communication 

The RP2350 can enumerate as a CDC USB device and appear as a com style serial port available to screen or putty. printf() functions are mapped to send character arrays to the computer, up to 64 bytes every millisecond. You do not need to enter a baud rate when opening the port. To map the printf() function, edit CMakeLists.txt:

```py
# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(my_project1 0)
pico_enable_stdio_usb(my_project1 1) # change from 0 to 1 to enable USB printf()
```

In the .c file, wait for the USB to enumerate, then you can printf() and scanf(). Note that scanf() is a blocking function that returns on the space or enter character.
```c
#include <stdio.h>
#include "pico/stdlib.h"
int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Start!\n");
 
    while (1) {
        char message[100];
        scanf("%s", message);
        printf("message: %s\r\n",message);
        sleep_ms(50);
    }
}
```
Test this out to see that the code is blocked until the port is opened on the computer.
## General IO pin usage

Initialize a pin to be used as general purpose IO with 
```c
gpio_init(PIN_NUM); // PIN_NUM without the GP
```
Set the direction of the pin with
```c
gpio_set_dir(PIN_NUM, GPIO_OUT or GPIO_IN);
```
Read the value of an input pin with
```c
gpio_get(PIN_NUM);
```
or set the value of an output pin with
```c
gpio_put(PIN_NUM, 0 or 1);
```
Test this out by initializing an external button and and LED circuit. If the button is pressed, turn on the LED, otherwise turn off the LED.

## Reading an analog voltage

To be able to use the ADC, first add hardware_adc to the target_link_libraries() list in CMakeLists.txt, after pico_stdlib.

In the .c file, add
```c
#include "hardware/adc.h"
```
Initialize the ADC with
```c
adc_init(); // init the adc module
adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
adc_select_input(0); // select to read from ADC0
```
Read the 12 bit value using
```c
uint16_t result = adc_read();
```
Test this out, build a potentiometer circuit from 0V to 3.3V and apply to pin ADC0. Read the pin every time the button is pressed and print out the 12 bit value. 

## Assignment
For this assignment, make a folder in your repo called HW3. Build code that
* Enables USB communication
* waits until the USB port has been opened
* Turns on an LED
* Waits until a button is pressed
* Turns off the LED
* Asks the user to enter a number of analog samples to take, between 1 and 100
* Reads the number entered by the user
* Reads the voltage on ADC0 that number of times at 100Hz
* Prints back the voltages in the units of volts
* then loops to ask the user again

Submit on Canvas a link to the folder in your repo and a short demo video.