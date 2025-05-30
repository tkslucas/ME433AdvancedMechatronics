#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "font.h"
#include "ssd1306.h"

// --- Pins & I2C Setup ---
#define INT_WATCH_PIN 17
#define SCL_PIN 5
#define SDA_PIN 4
#define I2C_PORT i2c0

// --- Device Addresses ---
#define OLED_ADDR 0x3C
#define IMU_ADDR  0x68
#define CMD_BYTE  0x00

// --- IMU Registers ---
#define CONFIG         0x1A
#define GYRO_CONFIG    0x1B
#define ACCEL_CONFIG   0x1C
#define PWR_MGMT_1     0x6B
#define PWR_MGMT_2     0x6C
#define INT_ENABLE     0x38
#define INT_STATUS     0x3A
#define ACCEL_XOUT_H   0x3B
#define TEMP_OUT_H     0x41
#define GYRO_XOUT_H    0x43
#define WHO_AM_I       0x75

// --- Globals ---
double accel_x = 0, accel_y = 0, accel_z = 0;
double gyro_x = 0, gyro_y = 0, gyro_z = 0;
double temp = 0;

uint8_t buf[14];
uint8_t oled_buf[513];
int imu_ready = 0;

// --- Prototypes ---
void ssd1306_send_command(uint8_t);
void ssd1306_setup(void);
void ssd1306_clear(void);
void ssd1306_update(void);
void set_pixel(bool, int, int);
void load_x_line(int, int);
void load_y_line(int, int);
void x_accel_update(void);
void y_accel_update(void);
void imu_init(void);
int i2c_check(void);
void gpio_callback();

// --- Main ---
int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_init(INT_WATCH_PIN);
    gpio_set_dir(INT_WATCH_PIN, GPIO_IN);

    while (!stdio_usb_connected()) ;
    sleep_ms(50);
    printf("Starting...\n");

    imu_init();
    ssd1306_setup();
    ssd1306_clear();
    sleep_ms(200);

    printf("I2C: %d\n", i2c_check());

    if (i2c_check() == 1) {
        printf("I2C communication works\n");
    } else {
        printf("Failed to communicate with IMU\n");
    }

    gpio_set_irq_enabled_with_callback(INT_WATCH_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    while (true) {
        if (imu_ready) {
            ssd1306_clear();

            uint8_t reg = ACCEL_XOUT_H;
            int res = i2c_write_blocking(I2C_PORT, IMU_ADDR, &reg, 1, true);
            if (res != 1) printf("I2C write error\n");

            res = i2c_read_blocking(I2C_PORT, IMU_ADDR, buf, 14, false);
            if (res != 14) printf("I2C read error\n");

            accel_x = (int16_t)(buf[0] << 8 | buf[1]) * 0.000061;
            accel_y = (int16_t)(buf[2] << 8 | buf[3]) * 0.000061;
            accel_z = (int16_t)(buf[4] << 8 | buf[5]) * 0.000061;
            temp    = (int16_t)(buf[6] << 8 | buf[7]) / 340.0 + 36.53;
            gyro_x  = (int16_t)(buf[8] << 8 | buf[9]) * 0.00763;
            gyro_y  = (int16_t)(buf[10] << 8 | buf[11]) * 0.00763;
            gyro_z  = (int16_t)(buf[12] << 8 | buf[13]) * 0.00763;

            printf("%.2f %.2f %.2f\n", accel_x, accel_y, accel_z);

            x_accel_update();
            y_accel_update();
            ssd1306_update();

            imu_ready = 0;
        }
        sleep_ms(5);
    }
}

// --- IMU Setup ---
void imu_init() {
    uint8_t cmd[2];

    cmd[0] = PWR_MGMT_1; cmd[1] = 0x80; i2c_write_blocking(I2C_PORT, IMU_ADDR, cmd, 2, false);
    sleep_ms(100);

    cmd[0] = PWR_MGMT_1; cmd[1] = 0x00; i2c_write_blocking(I2C_PORT, IMU_ADDR, cmd, 2, false);
    cmd[0] = GYRO_CONFIG; cmd[1] = 0x18; i2c_write_blocking(I2C_PORT, IMU_ADDR, cmd, 2, false);
    cmd[0] = ACCEL_CONFIG; cmd[1] = 0x00; i2c_write_blocking(I2C_PORT, IMU_ADDR, cmd, 2, false);
    cmd[0] = PWR_MGMT_1; cmd[1] = 0x21; i2c_write_blocking(I2C_PORT, IMU_ADDR, cmd, 2, false);
    cmd[0] = PWR_MGMT_2; cmd[1] = 0x80; i2c_write_blocking(I2C_PORT, IMU_ADDR, cmd, 2, false);
    cmd[0] = INT_ENABLE; cmd[1] = 0x01; i2c_write_blocking(I2C_PORT, IMU_ADDR, cmd, 2, false);
}

int i2c_check() {
    uint8_t reg = WHO_AM_I, id = 0;
    if (i2c_write_blocking(I2C_PORT, IMU_ADDR, &reg, 1, true) != 1) return 2;
    if (i2c_read_blocking(I2C_PORT, IMU_ADDR, &id, 1, false) != 1) return 3;
    printf("WHO_AM_I: 0x%X\n", id);
    return (id == 0x68) ? 1 : 0;
}

// --- ISR ---
void gpio_callback() {
    imu_ready = 1;
}

// --- OLED Commands ---
void ssd1306_send_command(uint8_t cmd) {
    uint8_t cmd_buf[2] = { CMD_BYTE, cmd };
    int res = i2c_write_blocking(I2C_PORT, OLED_ADDR, cmd_buf, 2, false);
    if (res < 2) printf("OLED command error\n");
}

void ssd1306_clear() {
    memset(oled_buf, 0, 513);
    oled_buf[0] = 0x40;
    if (i2c_write_blocking(I2C_PORT, OLED_ADDR, oled_buf, 513, false) != 513) {
        printf("OLED clear error\n");
    }
}

void ssd1306_setup() {
    sleep_ms(20);
    uint8_t cmds[] = {
        SSD1306_DISPLAYOFF, SSD1306_SETDISPLAYCLOCKDIV, 0x80,
        SSD1306_SETMULTIPLEX, 0x1F, SSD1306_SETDISPLAYOFFSET, 0x00,
        SSD1306_SETSTARTLINE, SSD1306_CHARGEPUMP, 0x14, SSD1306_MEMORYMODE,
        0x00, SSD1306_SEGREMAP | 0x1, SSD1306_COMSCANDEC, SSD1306_SETCOMPINS,
        0x02, SSD1306_SETCONTRAST, 0x8F, SSD1306_SETPRECHARGE, 0xF1,
        SSD1306_SETVCOMDETECT, 0x40, SSD1306_DISPLAYON
    };
    for (int i = 0; i < sizeof(cmds); i++) {
        ssd1306_send_command(cmds[i]);
    }
}

void ssd1306_update() {
    ssd1306_send_command(SSD1306_PAGEADDR);
    ssd1306_send_command(0);
    ssd1306_send_command(0xFF);
    ssd1306_send_command(SSD1306_COLUMNADDR);
    ssd1306_send_command(0);
    ssd1306_send_command(127);

    if (i2c_write_blocking(I2C_PORT, OLED_ADDR, oled_buf, 513, false) != 513) {
        printf("OLED update error\n");
    }
}

// --- Drawing ---
void set_pixel(bool state, int x, int y) {
    int page = (y - 1) / 8;
    int bit = (y - 1) % 8;
    int col = x - 1;
    if (page < 0 || page > 4 || col < 0 || col > 127) return;

    int i = page * 128 + col + 1;
    if (state) oled_buf[i] |= (1 << bit);
    else       oled_buf[i] &= ~(1 << bit);
}

void load_x_line(int x1, int x2) {
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    for (int i = x1; i <= x2; i++) set_pixel(true, i, 16);
}

void load_y_line(int y1, int y2) {
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    for (int i = y1; i <= y2; i++) set_pixel(true, 64, i);
}

void x_accel_update() {
    int gap = (int)(fabs(accel_x) / 1.5 * 63.0);
    int center = 64;

    if (accel_x < 0) {
        // Draw left instead of right
        load_x_line(center - gap, center);
    } else {
        // Draw right instead of left
        load_x_line(center, center + gap);
    }
}

void y_accel_update() {
    int gap = (int)(fabs(accel_y) / 1.3 * 15.0);
    int center = 16;

    if (accel_y > 0) {
        // Draw upward instead of downward
        load_y_line(center - gap, center);
    } else {
        // Draw downward instead of upward
        load_y_line(center, center + gap);
    }
}

