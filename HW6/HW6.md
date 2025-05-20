## Old I2C videos

[Intro to I2C video from 2020](https://youtu.be/t_cRklb0nw4)  
_In 2020 we used the MCP23017, a large chip that adds 16 more general purpose IO pins to the microcontroller. This year we'll use the MCP23008, which is very similar but adds only 8 pins._  
[IO expander video from 2020](https://youtu.be/aSqQmRLFnd0)  

## I2C

Inter-integrated circuit communication, or I2C, is another common type of digital communication. It uses only two pins, SDA and SCL, and is able to talk to up to 128 chips on the same bus. This reduces the pin count that you would need if you used SPI, but the downside is that the baud is typically 100kHz or 400kHz.  

SCL is the clock pin, controlled by the host, like the SCK pin in SPI. That leaves SDA as the only data pin, so it needs to be capable of switching from input to output depending on the direction of the communication. To prevent shorts from occurring, the pins use an open collector as output, so they require a pull-up resistor, typically 1k to 10k. 

Chips on the bus know which chip is being communicated with using an address system. Every device on the bus must have a unique 7bit address. The SCL and SDA pins idle high, due to the pull-up resistors. When the host wants to communicate, it brings SCL low, and transmits an 8bit byte containing the 7bit address of the chip it wants to talk to, and then switches the SDA pin to input. If the chip is present, it switches its SDA pin to output, and pulls the pin low, acknowledging the request. Every 8bit transfer will end in a 9th bit ack. If the ack does not come, the host might get stuck forever, waiting for a response.  

The 8th bit in the first transaction represents whether the host wants to write to or read from the device. The next byte sent is a register, the address that the host wants to write to or read from in the device. If writing, the host continues to send data, writing into the chip. If trying to read from a device, the host first writes the address of the chip with a write bit, then writes the register it wants to read from, then restarts the communication with the address of the chip with a read bit, followed by switching the SDA pin to input and reading bytes in and acking after each byte. 

Use the New C/C++ project wizard with the I2C checkbox checked to generate a project with the I2C pins enabled.  

To send data, use:

```c
i2c_write_blocking(i2c_default, ADDR, buf, 2, false);
```
where ADDR is the 7bit address of the chip, buf is an array of 8bit data you are sending, 2 is the length of the array, and false means you are only writing, not trying to read from the chip. The first byte in buf is the value of the register you are trying to change, the second byte is the value to want to write to that register.

To read data, use:

```c
i2c_write_blocking(i2c_default, ADDR, &reg, 1, true);  // true to keep master control of bus
i2c_read_blocking(i2c_default, ADDR, &buf, 1, false);  // false - finished with bus
```
where reg is the 8bit register you want to read from, and buf is the 8bit number you are getting from the chip.


## IO Expander

The [MCP23008](https://www.microchip.com/en-us/product/MCP23008) is a DIP chip that gives you 8 more general purpose IO pins. It is controlled by I2C, which uses two pins, so the overall circuit will net gain 6 controllable pins for your microcontroller. This can be useful for boards that don't have many pins to begin with.  

The MCP23008 has 3 address pins, so you could have 8 of the chips on the same I2C bus. That would provide 64 pins, using only 2 from the Pico! Now if you wanted to control 64 LEDs or read 64 buttons there are lots of other, probably more efficient, circuits, but the MCP23008 is relatively straight forward.   

Connect the MCP23008 Vdd to 3.3V and Vss to GND. Connect the RESET pin to 3.3V. Connect the SDA to a 10k pull-up resistor, and the SCL to a 10k pull-up resistor, and connect the pins to the Pico I2C0 SDA and I2C0 SCL. Connect the MCP23008 A0, A1, and A2 pins to GND (or whatever address you'd like to make).  

**Note that by connecting the RESET pin to 3.3V, the only way to reset the chip is to kill power to the whole board. Resetting is sometimes necessary because if your code happens to be in the middle of communicating with the MCP23008 when the Pico is reset, the Pico code will start fresh but the MCP23008 might have been in the middle of communication and will not know that the Pico started over, so it will never respond to new communication and the Pico will get stuck in an infinite loop.** Always put some kind of heart-beat in your code so you can tell that the Pico is stuck in an infinite loop and you know to do a full power reset. We could manually control the RESET pin with an output pin from the Pico, but that would steal another Pico pin, defeating the purpose of gaining pins with the MCP23008.  

To test the chip, add an LED and a 330 ohm resistor to GND from pin GP7, and add a button from pin GP0 to GND with a 10k pull-up resistor.  

The goal is to read from GP0, and if the button is pushed, turn on GP7, else turn off GP7. Blink the builtin green LED that is connected directly to the Pico at some frequency as a heart-beat, so that you know the Pico is running, and hasn't crashed due to the I2C getting our of sync. 

I2C is very tricky to debug because it is hard to tell if the chip initialization, the read function, or the write function is the problem. Break the assignment into 2 parts: chip initialization and blink the LED on GP7, then read from GP0 and control GP7.  

The chip must be initialized so that GP7 is an output pin and GP0 is input. You can make all the other pins inputs. Write an unsigned char to IODIR to set the direction of each pin.   

Set the bits in OLAT to turn the pins on or off, and read from GPIO to know if the button is pushed or not. Note that there are no structures like LATAbits. or PORTBbits. built into the code, so you'll probably need some bitshifting and ANDing and ORing to identify individual bits from the unsigned chars.   

I suggest creating general purpose write and read functions, like 'void setPin(unsigned char address, unsigned char register, unsigned char value)', and 'unsigned char readPin(unsigned char address, unsigned char register)', we'll use them or similar functions in future assignments.  

Make a folder in your repo called HW6 with your code and upload a link to the folder in Canvas. Record a video demonstrating your button push turning on the LED and upload to Canvas. 