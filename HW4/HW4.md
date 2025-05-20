Note: I have some old videos on the next few assignments when we still used the PIC32 in ME433. There may be some useful bits in them even though we are now using the RP2350.

[Old intro to PIC32 SPI video 2020](https://youtu.be/bNL6Z6_2a0Y)   

[Old using an SPI DAC with PIC32 video 2020](https://youtu.be/0-iUEn8wPMI) (using the 12-bit version of the DAC chip)

## Digital to Analog Conversion

Most microcontrollers have an Analog-to-Digital converter (ADC), so that they can read analog voltages as digital numbers. Relatively few microcontrollers have the opposite peripheral, a Digital-to-Analog converter, or DAC, that can make a specific voltage. Perhaps this is because analog data can contain more noise than the equivalent transfer of information with digital communication, but there are still plenty of devices that need an analog input to function, like motor controllers, function generators, power supplies, and most often, speaker amplifiers.  

The RP2350 does not have a built in DAC peripheral, so to make a voltage, we'll use an external chip. There are a variety of chips available, with different resolutions, update rates, noise levels, current outputs, and number of output channels. The trick to using them is in the method of communication, how do you instruct the chip to make a specific voltage at a specific time?  

Digital communication can come from several peripherals that the microcontroller does have. Serial, or UART, uses one pin to send data (TX) and another to receive (RX). I2C also uses two pins, but one is a dedicated clock, and the other is used for data. When you have a dedicated clock pin the communication is called synchronous, and can usually be transferred at much higher rates. SPI is synchronous, with a clock pin, a dedicated output pin, and a dedicated input pin, plus one pin to each chip you are talking to.  

We will start our investigation into digital communication protocols using an SPI DAC. This chip cannot talk back to the Pico, so it simplifies the process of debugging a little. When you ask the chip to create an output voltage, if it doesn't do it, then you know there is something wrong with your SPI writing function. Next we will use another type of chip that can talk back, and debugging will be a little harder.  

You can find example code for the Pico SPI peripheral in the SPI examples for the BME280 and MAX7219 chips, or look at the Raspberry Pi git account: [https://github.com/raspberrypi/pico-examples/blob/master/spi/spi_flash/spi_flash.c](https://github.com/raspberrypi/pico-examples/blob/master/spi/spi_flash/spi_flash.c).    

The default pins for the SPI0 peripheral are GP16 (MISO or SPIRx), GP18 (SCK), and GP19 (MOSI or SPITx). Any digital output can be used as CS.  

SPI is initialized with:
```c
spi_init(spi_default, 1000 * 1000); // the baud, or bits per second
gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
```

And data is written with:
```c
cs_select(PIN_CS);
spi_write_blocking(SPI_PORT, data, len); // where data is a uint8_t array with length len
cs_deselect(PIN_CS);
```

Using these functions to control the CS pin:
```c
static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}
```

Use the [datasheet for the MCP4912](https://www.microchip.com/en-us/product/mcp4912) (note we have the 10 bit version) to generate a 2Hz sine wave and a 1Hz triangle wave. Both should range from 0V to 3.3V. The update rate of the DAC should be at least 50 times faster than the frequency of the signal you are producing.  

Make a folder called HW4 in your repo and upload your code, as well as a screenshot of the output voltages on an oscilloscope, showing at least 3 cycles of each signal.  

Suggestions:  
- Be careful inserting the chip into your breadboard. Press each side of your chip against a table to bend the pins more perpendicular to the body of the chip so that it fits easier into the breadboard.  
- Set the baud of the SPI to something really slow, like 12kHz, so you can see the CS, SCK, and SDO values on nLab (on a real oscilloscope you can go full speed). It is helpful for debugging. Once it is working, you can max out the baud. The nLab has a bandwidth of about 100kHz, so it can't see signals faster than 100kHz, but a real oscilloscope's bandwidth is > 10MHz.  
- Test one thing at a time! Don't write the entire assignment and assume it will work, break it up into smaller parts and test as you go: Write a function that takes the channel and the voltage (as an unsigned short, 0-1023) as inputs. At first, just hard code the entire 16 bits to send to the DAC, with something like 1.65V as the output, to test your SPI writes. Then actually use the channel and voltage.