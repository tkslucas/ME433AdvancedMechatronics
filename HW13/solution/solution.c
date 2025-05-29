#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

#include "ssd1306.h"
#include "font.h"

// I2C
#define SCL_PIN 5
#define SDA_PIN 4
#define I2C_PORT i2c0

extern char ssd1306_buffer[513];

static void pico_init_all();
static void io_expander_init();
void display_adc(int x, int y);
void display_voltage(int x, int y);
void display_frames_per_second(int x, int y);
static void pico_set_led(bool led_state);
static void WritePin(uint8_t advice, uint8_t register, uint8_t data);
static uint8_t ReadPin(uint8_t advice, uint8_t register);

// FPS
static float frames_per_second;
uint32_t t_start;
uint32_t t_end;
uint32_t t_diff;

// ADC
uint16_t adc_count;
float adc_voltage;

// Display
int col_num;

int main() {
    pico_init_all();

    while (true) {
        t_start = to_us_since_boot(get_absolute_time());  
        adc_count = adc_read();
        ssd1306_clear();

        display_adc(1, 1);
        display_voltage(1, 2);
        display_frames_per_second(30, 4);
        ssd1306_update();

        pico_set_led(!gpio_get(PICO_DEFAULT_LED_PIN)); // Heartbeating led

        sleep_ms(1000); // 1Hz

        t_end = to_us_since_boot(get_absolute_time());  
        t_diff = t_end - t_start;
        frames_per_second = 1000000.0 / (float)t_diff;
    }
}

static void pico_init_all() {
    // stdio
    stdio_init_all();

    // CDC USB, waits until USB port has been opened
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("CDC USB Start!\n");

    // Pico LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // I2C
    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);

    // OLED Display
    ssd1306_setup();
    ssd1306_clear();
}

void display_adc(int x, int y){
    char str[25];
    sprintf(str, "ADC counts = %d", adc_count);
    col_num = x;
    for (int i = 0; i < strlen(str); i++){
        if (col_num + 5 > 128){
            if (y < 4){
                y ++;
            } else {
                printf("Out of bounds");
                break;
            }    
            col_num = 0;
            x = 1;
        }
        int position = 128*(y-1) + col_num;
        col_num +=5;
        memcpy(&ssd1306_buffer[position], ASCII[(uint8_t)(str[i])-32], 5);
    }
}

void display_voltage(int x, int y){
    char str[25];
    adc_voltage = (float)adc_count/4096 * 3.3;
    sprintf(str, "ADC Voltage: %.2fV", adc_voltage);
    col_num = x;
    for (int i = 0; i < strlen(str); i++){
        if (col_num + 5 > 128){
            if (y < 4){
                y ++;
            } else {
                printf("Out of bounds");
                break;
            }    
            col_num = 0;
            x = 1;
        }
        int position = 128*(y-1) + col_num;
        col_num += 5;
        memcpy(&ssd1306_buffer[position], ASCII[(uint8_t)(str[i])-32], 5);
    }
}

void display_frames_per_second(int x, int y){
    char str[25];
    sprintf(str, "FPS: %.5f", frames_per_second);
    col_num = x;
    for (int i = 0; i < strlen(str); i++){
        // Boundary checking and row shifting
        if (col_num + 5 > 128){
            if (y < 4){
                y ++;
            } else {
                printf("Out of bounds");
                break;
            }    
            col_num = 0;
            x = 1;
        }
        // Every row means 128 byte and every colume means 1 bytes 
        int position = 128*(y-1) + col_num;
        // Every ASCII char needs 5 columes to display
        col_num += 5;
        // Match the current char to its 5 bytes displaying array and then copy to the ssd1306_bufferfer
        memcpy(&ssd1306_buffer[position], ASCII[(uint8_t)(str[i])-32], 5);
    }
}

static void WritePin(uint8_t device_addr, uint8_t reg_addr, uint8_t data){
    uint8_t I2C_buf[2];
    I2C_buf[0] = reg_addr;
    I2C_buf[1] = data;

    int result = i2c_write_blocking(I2C_PORT, device_addr, I2C_buf, 2, false);
    if (result < 0){
        printf("Error occurs druing Writing Communication!\r\n ");
    }
}

static uint8_t ReadPin(uint8_t device_addr, uint8_t reg_addr){
    uint8_t I2C_buf;
    int result;

    result = i2c_write_blocking(I2C_PORT, device_addr, &reg_addr, 1, true);
    if (result < 0){
        printf("Error occurs druing Writing Communication!\r\n ");
    }

    result = i2c_read_blocking(I2C_PORT, device_addr, &I2C_buf, 1, false);
    if (result < 0){
        printf("Error occurs druing Reading Communication!\r\n ");
    }

    return I2C_buf;
}

static void pico_set_led(bool led_state){
    gpio_put(PICO_DEFAULT_LED_PIN, led_state);
}