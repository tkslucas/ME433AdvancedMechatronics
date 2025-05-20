Setup your Raspberry Pi Pico 2 in a breadboard, with 3.3V and ground in all of the rails. Place a button from the RUN pin to GND (this is the reset button). To put the board in bootloader mode, press and hold the RUN button, press and hold the BOOT button, let go of the RUN button, then let go of the BOOT button. The board will appear as a thumb drive. Build a red LED circuit from 3.3V to GND to act as a power indicator.

Here is the pinout of the board: [Pico 2 pinout](https://www.raspberrypi.com/documentation/microcontrollers/images/pico-2-r4-pinout.svg)

Use the Raspberry Pi Pico VSCode extension to make a "New Project from Example". Place the project in a folder in your repo called HW2. Use blink as an example.  

Compile the example and upload to the board. Change the blink delay from 250ms to 1000ms and verify that everything is working.

Use the example code from blink, hello_usb, and hello_gpio_irq, with a button circuit and an LED circuit, to toggle the LED every time the button is pressed, and print how many times the button was pressed to the computer. Use a terminal emulator like screen or putty to see the USB data.

Make a short video demonstrating your code, upload your code to github, and submit a link to your repo and upload the video to Canvas.

Edit:
The printf function does not map to print to the USB port by default. For a project to create a USB port and print to screen/putty, you must add the following lines to the CMakeLists.txt file in your project (see hello_usb for an example):

```py
# enable usb output, disable uart output
pico_enable_stdio_usb(hello_gpio_irq 1)
pico_enable_stdio_uart(hello_gpio_irq 0)
```