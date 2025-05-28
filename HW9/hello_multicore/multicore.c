#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"

#define FLAG_VALUE 123

uint16_t adc_count;
int command;

void core1_entry() {
    uint32_t g;
    multicore_fifo_push_blocking(FLAG_VALUE);
    g = multicore_fifo_pop_blocking();
    
    if (g != FLAG_VALUE) {
        printf("Core 1 initialization failed\n");
    } else {
        printf("Core 1 ready\n");
    }

    while (true) {
        g = multicore_fifo_pop_blocking();
        
        switch (g) {
            case 0:  // Read ADC0
                adc_count = adc_read();
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;

            case 1:  // Turn on GPIO15
                gpio_put(15, 1);
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;
            
            case 2:  // Turn off GPIO15
                gpio_put(15, 0);
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;

            default:
                break;
        }

        tight_loop_contents();
    }
}

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(20);
    }
    printf("Multicore example started\n");

    adc_init(); 
    adc_gpio_init(26); 
    adc_select_input(0); 

    gpio_init(15);
    gpio_set_dir(15, GPIO_OUT);
    gpio_put(15, 0);

    uint32_t g;
    multicore_launch_core1(core1_entry);
    sleep_ms(50);
    
    g = multicore_fifo_pop_blocking();
    multicore_fifo_push_blocking(FLAG_VALUE);

    if (g != FLAG_VALUE) {
        printf("Core 0 initialization failed\n");
    } else {
        multicore_fifo_push_blocking(FLAG_VALUE);
        printf("Core 0 ready\n");
    }

    while (true) {
        printf("Enter command (0-2): ");
        scanf("%d", &command);
        printf("\n");

        switch (command) {
            case 0:  // Read ADC
                multicore_fifo_push_blocking(command);
                g = multicore_fifo_pop_blocking();
                if (g != FLAG_VALUE) {
                    printf("ADC read error\n");
                } else {
                    printf("ADC voltage: %.2f V\n", (float)adc_count/4096.0*3.3);
                }
                break;
            
            case 1:  // LED on
                multicore_fifo_push_blocking(command);
                g = multicore_fifo_pop_blocking();
                if (g != FLAG_VALUE) {
                    printf("LED on error\n");
                } else {
                    printf("LED on\n");
                }
                break;

            case 2:  // LED off
                multicore_fifo_push_blocking(command);
                g = multicore_fifo_pop_blocking();
                if (g != FLAG_VALUE) {
                    printf("LED off error\n");
                } else {
                    printf("LED off\n");
                }
                break;
            
            default:
                printf("Invalid command\n");
                break;
        }
    }
}