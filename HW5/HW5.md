Let's play with SPI some more. You may need some of the SPI code from [HW4](https://github.com/ndm736/ME433_2025/wiki/HW4).

## Math and Timing

First, let's investigate the RP2350 a little more. Here is the [datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).  

The RP2350 contains the [ARM Cortex M33 CPU design](https://developer.arm.com/Processors/Cortex-M33), which contains an FPU, or floating point unit, using the [IEEE 754 standard](https://en.wikipedia.org/wiki/IEEE_754). According to Section 3.6.4 of the datasheet, the Pico SDK enables use of the FPU by default, so math with float type variables should be much faster compared to a microcontroller that must use an algorithm to do floating point math.  

Let's do a comparison to see how long different types of math take:

```c
    volatile float f1, f2;
    printf("Enter two floats to use:");
    scanf("%f %f", &f1, &f2);
    volatile float f_add, f_sub, f_mult, f_div;
    f_add = f1+f2;
    f_sub = f1-f2;
    f_mult = f1*f2;
    f_div = f1/f2;
    printf("\nResults: \n%f+%f=%f \n%f-%f=%f \n%f*%f=%f \n%f/%f=%f\n", f1,f2,f_add, f1,f2,f_sub, f1,f2,f_mult, f1,f2,f_div);
```
I'm making the variables volatile here so that the complier's optimization step doesn't try to skip the math (since the variables are never used anywhere else, the compiler might just skip the whole thing!)  

Time can be measured with the function:
```c
absolute_time_t t1 = get_absolute_time();
```

And then converted into microseconds as an unsigned 64bit int, using:
```c
uint64_t t = to_us_since_boot(t1)
```

To print this large type, use:
```c
printf("t = %llu\n", t);
```

The RP2350 on the Pico 2 runs at 150MHz, or 6.667ns per clock cycle, so measuring math in microseconds isn't actually that useful, the chip is too fast! **Edit the code above so that that each equation happens 1000 times in a for loop, measure the time the loop takes and divide by 1000, and convert to clock cycles. Print out how many clock cycles each takes (I got about 7 for addition, subtraction, and multiplication, and 20 for division) take a screenshot and include in your repo.**

## How to write to external RAM

The RP235x series of chips have 520kB of RAM, and either no built in flash memory (x=0) or 2MB (x=4). The Pico 2, with a RP2350, has 4MB of external flash, and you can find other breakout boards with as much as 16MB. 

So we are not in short supply of memory, but that is not always the case. Memory is often the most expensive part of a chip, because it [can take up a lot of space](https://siliconpr0n.org/map/raspberry-pi/rp2-b0/mcmaster_s1-9_vc60x/), and thus fewer chips can be made from a single [wafer of silicon](https://en.wikipedia.org/wiki/Wafer_(electronics)#/media/File:Wafer_2_Zoll_bis_8_Zoll_2.jpg).  

We will practice using an external memory chip, the [23k256 external SRAM](https://ww1.microchip.com/downloads/en/DeviceDoc/22100F.pdf), with 256kbit of RAM. Remember that RAM is volatile memory, the contents are lost when power is removed. Nonvolatile memory, like Flash EEPROM, would remember the contents when power is removed.

The "256kbit" is organized as 32768 8bit addresses. It uses all the SPI pins, plus has a HOLD pin to prevent changes. We don't need the HOLD, so you can wire it to 3.3V. 

To write into memory, first lower the CS pin, then send an 8bit instruction, then a 16bit address, then the data you want to write there, and raise the CS pin when you are done with the transaction.   

To read from memory,  lower the CS pin, then send an 8bit instruction, then a 16bit address, then read as many bytes as you want, and raise the CS pin when you are done with the transaction.  

The 8bit instructions are 0b00000010 to write and 0b00000011 to read.  

There are 3 operation modes for accessing data. In byte operation, only one byte can be read or written in one transaction. In page operation, the address wraps around a page of 1024 continuous bytes. In sequential operation, the address wraps around the entire memory space. 

To set the mode, send the instruction 0b00000001, followed by 0b00000000 for byte operation, 0b10000000 for page operation, and 0b01000000 for sequential operation.  

The data you put into RAM doesn't have any structure, just bytes. But you might want to put a float in there, which is a 32bit, or 4byte type. To store a float into RAM, you must identify each byte, send, and reassemble when it is read back out.  

## Assignment 

During initialization, in a loop, load 1000 floats into external RAM, representing a single cycle of a sine wave from 0V to 3.3V.

In the infinite while loop, read one float from external RAM, write the value to the DAC from [HW4](https://github.com/ndm736/ME433_2025/wiki/HW4), and delay one millisecond, to create a 1Hz sine wave output.

Take a screenshot of the output, push the code into your repo in a folder called HW5, and submit the link to your repo and the screenshot to Canvas, with the screenshot of the number of cycles it takes to do float math.

## Hint 1

In C, you cannot use bitshifting on a float, so how do you break it into four chars? One way is to double cast the memory with a Union. This means that for one memory address, the bits that are there can be interpreted two different ways:

```c
union FloatInt {
    float f;
    uint32_t i;
};
```

So that now if you:
```c
union FloatInt num;
num.f = 1.23;
```

You find the leftmost 8bits with:
```c
(num.i>>24)&0xFF
```

## Hint 2

In addition to initializing the SPI peripheral and pins, now you need to initialize the chip. Write a function to do that, like:

```c
void spi_ram_init();
```

Then write functions for writing and reading, using sequential mode. The input and output could be a float, or an array containing the bytes that make up the float, or an array of floats. There's lots of ways to break this assignment up, tackle it one byte at a time.
