## PWM, RC Servos, and Addressable LEDs

### PWM on the RP2040

Page 1075 of the [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf) describes the PWM peripheral.  

Every pin on the RP2350 can output PWM (and also input PWM). But not all independently. Table 1129 in section 12.5.2 shows that there are 12 PWM slices, each with 2 channels, A and B. The channels must have the same frequency, but can have different duty cycles. Channels are on adjacent GPIO pins. In general, it is easiest to put each PWM on its own slice, so avoid using GPIO pins that are right next to each other for PWM.  

The PWM counters use the system clock as a source, which is 150MHz. This source can be slowed down by a divider (for some reason a float?) from 1 to 255. The timer rolls back to 0 when it hits the wrap parameter, an unsigned 16 bit number. The duty cycle is set as a number from 0 to the wrap value. This means that you should try to maximize the wrap value to get the most unique duty cycles, but it must be less than 65535.  

This makes the frequency of the PWM 150MHz / divider / wrap.  

In this example, the PWM frequency is 1kHz. Using a divider of 3 makes the wrap 50000, thats lots of resolution!  

```c
  #define LEDPin 25 // the built in LED on the Pico
  gpio_set_function(LEDPin, GPIO_FUNC_PWM); // Set the LED Pin to be PWM
  uint slice_num = pwm_gpio_to_slice_num(LEDPin); // Get PWM slice number
  float div = 3; // must be between 1-255
  pwm_set_clkdiv(slice_num, div); // divider
  uint16_t wrap = 50000; // when to rollover, must be less than 65535
  pwm_set_wrap(slice_num, wrap);
  pwm_set_enabled(slice_num, true); // turn on the PWM

  pwm_set_gpio_level(LEDPin, wrap / 2); // set the duty cycle to 50%
```

Don't forget to include the library with #include "hardware/pwm.h" and add hardware_pwm in the CMakeLists.txt file.

### RC Servos
An RC servo is a motor with built in position control. The output shaft is a spline, and it comes with a baggie filled with a variety of horns that fit onto the spline. The Brown wire is GND, the Red wire is supply voltage, and the Orange wire is a PWM signal. The supply voltage is usually rated from 4.8V to 6V. For this example, use 5V from VUSB. Use the extra long header pins to make a plug for your breadboard.  

A Servo PWM signal has to be 50Hz, with a minimum duty cycle corresponding to a 0.5mS pulse and a maximum duty cycle of 2.5mS. 50Hz is 20mS, so the duty cycle can range from 2.5% to 12.5%. A potentiometer is used to measure the output angle of the motor, so the Servo can only rotate 180 degrees. There is often a mechanical stop preventing the motor from rotating outside the range of 0 to 180 degrees.  

Connect the RC servo to the Pico. Write a function to initialize the PWM to 50Hz, and a function to set the angle of the servo. Test the code by calling the function so that the servo moves from 0 degrees to 180 degrees and back in 4 seconds. Notice that the motor is really taking tiny steps because it has a very "stiff" PID controller. The smaller and more often your steps, the finer the motion will be.  

The 0.5mS to 2.5mS pulse sizes are estimates - some brands of servos use 1mS to 2mS, so you may need to adjust these values to get exactly 0 to 180 degrees.  

### Addressable LEDs

To add a little color to our circuits we'll use some WS2812B addressable color LEDs in [an 8mm package from Sparkfun](https://www.sparkfun.com/led-rgb-addressable-pth-8mm-diffused-5-pack.html). The WS2812B is also called a [Neopixel](https://www.adafruit.com/category/168) and comes on sticks, rings, matrices, and infinitely long lengths of flexible tape.  

[Traditional color LEDs](https://www.sparkfun.com/products/105) are very pin intensive. Each LED is really a red, green, and blue LED cast inside the same plastic lens. The intensity of each can be set using a unique PWM pin, but a microcontroller has a limited number PWM pins, so it can't drive many full color LEDs! It would take a lot of multiplexing to drive a lot of color LEDs.  

WS2812B are a nice solution. They have a built-in controller that takes a single digital signal to set the brightness of the red, green, and blue LEDs, and if you immediately send another color, the data is passed-through to the next WS2812B. That means you can chain WS2812Bs in series and control each color and brightness uniquely with only one pin from the microcontroller!  

The [datasheet](https://cdn.sparkfun.com/assets/a/b/1/e/1/DS-12877-LED_-_RGB_Addressable__PTH__8mm_Diffused__5_Pack_.pdf) shows the pinout on page 3. WS2812B technically run on 5V, but will also run on 3.3V. **There is no reverse voltage protection on the WS2812B, so if you plug it in backwards, you will damage the LED. It will get extremely hot, and possibly pop when the plastic lens melts on the inside faster than the outside, so be very careful!** Wire up 4 WS2812B, with the first connected to a digital pin from the Pico, and data-out of the first chip to data-in of the second chip, and so on.    

It takes 24 bits to set the color of a WS2812B, 8 bits for each of the red, green, and blue LEDs, MSB first. The weird thing about the communication is that, because it is asynchronous and doesn't use a clock, the timing is very important, and every bit contains both a high voltage and a low voltage. A logic One bit is high for 1.36uS and low for 0.35uS, and a logic Zero is high for 0.35uS and low for 1.36uS, each deviating by no more than 0.15uS. If there is ever more than 50uS of low, the pass-through starts over (so this is the reset time).  

Once you set the color of a WS2812B it stays that color until you give it a new color. When the chips first power on they are sometimes bright blue, so as soon as you power them you probably need to set them all off.  

The Pico does not have a peripheral like UART or I2C that can control the WS2812B, in fact no microcontroller has this peripheral, the signal is totally made up. Most libraries "bit-bang" the protocol to control the colors. This means a very manual method of turning a pin on, delaying, turning the pin off, delaying, etc. This is annoying because it will take a lot of CPU time, and even with timer based ISRs, difficult to get the highs and lows accurate enough to support large strings of LEDs.  

But, the RP2350 has a trick up it's sleeve, a special feature called [programmable IO](https://blues.com/blog/raspberry-pi-pico-pio/), or PIO. A PIO block is a set of buffers connected to state machines, essentially small CPUs that can run independently of the main CPU. The SM have only a few possible instructions, and must be programmed in an assembly-like language, but chaining them together can create all kinds of low level protocols, like SPI, UART, I2C, and PWM, and even HDMI, and the digital signal timing necessary for WS2812B. And since they run independently of the main CPU, once you load data into their FIFO, the PIO run themselves, leaving you to move on to other code.  

Luckily we don't need to write the protocol from scratch. Use the VSCode extension to generate the PIO_WS2812 example. Change the WS2812_PIN to the number of the GP pin you are using to output data to the first WS2812Bs DIN pin, and change NUM_PIXELS to 4. Also change the order of the colors in the function urgb_u32(), so that the green byte is shifted by 8 and the red by 16 (maybe the Sparkfun LEDs are knockoffs, and to get around licensing fees they changed the order of the colors?). 

No sample code is perfect, this example is a little more complicated than it needs to be. If you run it, a variety of flashing LEDs will occur. What we really want is something a little more simple, just a function that sets the color of each LED. If would look something like this:  

```c
int i;
for(i=0;i<NUM_PIXELS;i++){
    put_pixel(pio, sm, urgb_u32(r[i], g[i], b[i])); // assuming you've made arrays of colors to send
}
sleep_ms(1); // wait at least the reset time
```

Get rid of all the complicated functions and try to set each LED to a different color with the code above.  

Rather than setting color by RGB, a popular way to control LEDs is by hue, saturation, and brightness, or HSB. Basically, you can calculate the RGB based on the [color wheel](https://en.wikipedia.org/wiki/Color_wheel). Then you can slide from one color to another without changing brightness, which would be distracting.  

It may be convenient to invent a new data to store the RGB values, like:  

```c
// link three 8bit colors together
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} wsColor; 
```
Then use this HSB to RGB function to create a color:  

```c
// adapted from https://forum.arduino.cc/index.php?topic=8498.0
// hue is a number from 0 to 360 that describes a color on the color wheel
// sat is the saturation level, from 0 to 1, where 1 is full color and 0 is gray
// brightness sets the maximum brightness, from 0 to 1
wsColor HSBtoRGB(float hue, float sat, float brightness) {
    float red = 0.0;
    float green = 0.0;
    float blue = 0.0;

    if (sat == 0.0) {
        red = brightness;
        green = brightness;
        blue = brightness;
    } else {
        if (hue == 360.0) {
            hue = 0;
        }

        int slice = hue / 60.0;
        float hue_frac = (hue / 60.0) - slice;

        float aa = brightness * (1.0 - sat);
        float bb = brightness * (1.0 - sat * hue_frac);
        float cc = brightness * (1.0 - sat * (1.0 - hue_frac));

        switch (slice) {
            case 0:
                red = brightness;
                green = cc;
                blue = aa;
                break;
            case 1:
                red = bb;
                green = brightness;
                blue = aa;
                break;
            case 2:
                red = aa;
                green = brightness;
                blue = cc;
                break;
            case 3:
                red = aa;
                green = bb;
                blue = brightness;
                break;
            case 4:
                red = cc;
                green = aa;
                blue = brightness;
                break;
            case 5:
                red = brightness;
                green = aa;
                blue = bb;
                break;
            default:
                red = 0.0;
                green = 0.0;
                blue = 0.0;
                break;
        }
    }

    unsigned char ired = red * 255.0;
    unsigned char igreen = green * 255.0;
    unsigned char iblue = blue * 255.0;

    wsColor c;
    c.r = ired;
    c.g = igreen;
    c.b = iblue;
    return c;
}
```

### Assignment

Build from the PIO_WS2812 example. In a loop that takes 5 seconds, loop through every color on the color wheel, with each LED being offset by 360/4 degrees, to create a walking rainbow effect. Also write a function that controls the angle of the RC servo with PWM, and have it move from 0 to 180 degrees every 5 seconds, at the same time as the LEDs cycle through the rainbow.  

Upload the code to your repo in a folder called HW8, and record a short video and submit to Canvas.  


