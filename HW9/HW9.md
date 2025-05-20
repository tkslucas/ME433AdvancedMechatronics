
### Multicore

The RP2350 has four CPUs, of which you can use two ARM or two RISCV at any time. RISCV is the more recently developed technology, and probably on this chip for more educational purposes than anything else, but maybe RISCV will be an ARM killer and more popular in a few years.  

The ARM CPUs are named Core 0 and Core 1. So far we have only used Core 0, and Core 1 has been asleep. You may want to use both cores when it makes sense in your project to separate functionality, like sensing from communication or lots of math. 

The complication that arises with using multiple cores is sharing memory. How does one core safely change memory that the other core may access? For instance, writing the value of a long or double takes 2 clock cycles, so what if Core 0 writes half a variable to RAM when Core 1 starts to read it? These types of problems are called [race conditions](https://en.wikipedia.org/wiki/Race_condition), and are just one of the many headaches you can run into while implementing code across several cores.  

The RP2350 simplifies the process by placing shared memory in two RAM FIFO buffers between the cores, one from Core 0 to Core 1, and one from Core 1 to Core 0. Data is "pushed", or written, into one FIFO by a core, and then can be "popped", or read, out in the other core.  

Build the example hello_multicore from the VSCode Pico extension to follow along:  

You'll need to add the USB printf() functionality, with the pause to wait for the port to open
In CMakeLists.txt:
```c
pico_enable_stdio_uart(hello_multicore 0)
pico_enable_stdio_usb(hello_multicore 1)
```
And in main():
```c
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
```

In main(), the function
```c
multicore_launch_core1(core1_entry);
```
enables Core 1, using the function core1_entry() as the main() function for Core 1. 

The core1_entry() function:
```c
void core1_entry() {

    multicore_fifo_push_blocking(FLAG_VALUE); // write back to core0

    uint32_t g = multicore_fifo_pop_blocking(); // read from core0

    if (g != FLAG_VALUE)
        printf("Hmm, that's not right on core 1!\n");
    else
        printf("Its all gone well on core 1!");

    while (1)
        tight_loop_contents(); // do nothing forever
}
```
Core 1 wakes up and waits for a message from Core 0, then sends a message to Core 0.
Note that the infinite while(1) loop uses the function tight_loop_contents(), which is a bunch of nops() to prevent the loop from being optimized away by the compiler.  

So now Core 0 sits there, waiting for a message from Core 1 
```c
    uint32_t g = multicore_fifo_pop_blocking();

    if (g != FLAG_VALUE)
        printf("Hmm, that's not right on core 0!\n");
    else {
        multicore_fifo_push_blocking(FLAG_VALUE);
        printf("It's all gone well on core 0!");
    }
```

There is an interrupt system that would allow you to skip the blocking() functions, but the example, multicore_fifo_irqs, doesn't work! It appears to be a bug with the interrupt system, so we'll skip it for now. That means your code will have to use a polling system to know if the cores are ready to share data with each other.   

### Unknowns

This is the first microcontroller chip I've tried with multicore (the ESP32 series is another multicore chip, usually bluetooth or wifi is offloaded to the second core). I can imagine all kinds of potential problems, like:
* what happens when each core tries to printf() at the same time?
* what happens if one core sets a pin high when the other core sets it low?
* what happens if one core crashes (divide by zero, memory out of bounds, etc)?
The answer is probably just don't do those things. Don't share pins across the cores, only printf() from one core. Build up your flagging system if you need more complicated behavior. 

### Assignment

Work from the hello_multicore example. 

On Core 0, do all of the communication. One Core 1, do all the pin stuff. Send flags between the cores to indicate when data in RAM is ready to be accessed without possible race conditions.   

* When the user sends the letter 0, return the voltage on pin A0.
* When the user sends the letter 1, turn on an LED on GP15.
* When the user sends the letter 2, turn off the LED on GP15.

Core 0 should do the scanf() and printf() stuff. Core 1 should initialize the pins, and when prompted by Core 0, do something to them and store a value in a global variable. The flags sent back and forth with push() and pop() should tell each core what to do.  

Place the code in your repo in a folder called HW9, record a short demo video and submit to Canvas.
