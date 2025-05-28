#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

#define BUTTON_UP     2
#define BUTTON_LEFT   3
#define BUTTON_DOWN   4
#define BUTTON_RIGHT  5
#define BUTTON_MODE   16
#define LED_MODE      15

typedef enum { REGULAR_MODE, REMOTE_MODE } mouse_mode_t;
static mouse_mode_t current_mode = REGULAR_MODE;

enum { BLINK_NOT_MOUNTED = 250, BLINK_MOUNTED = 1000, BLINK_SUSPENDED = 2500 };
static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);
void gpio_mode_button(uint, uint32_t);

void gpio_init_input(uint pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);
  gpio_pull_up(pin);
}

void gpio_init_output(uint pin, bool value) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, value);
}

void button_led_init(void) {
  uint pins[] = { BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_MODE };
  for (int i = 0; i < 5; ++i) gpio_init_input(pins[i]);
  gpio_init_output(LED_MODE, 0);
}

bool debounce(uint pin) {
  if (!gpio_get(pin)) {
    sleep_ms(5);
    return !gpio_get(pin);
  }
  return false;
}

int8_t handle_button(uint pin, uint32_t* hold) {
  if (debounce(pin)) return (int8_t)(1 + 0.1 * ++(*hold));
  *hold = 0;
  return 0;
}

static void send_hid_report(uint8_t report_id, uint32_t btn) {
  if (!tud_hid_ready()) return;

  static bool has_key_kbd = false, has_key_cons = false, has_key_gp = false;
  static uint32_t h_up = 0, h_down = 0, h_left = 0, h_right = 0;
  static double step = 0.0;

  switch (report_id) {
    case REPORT_ID_KEYBOARD: {
      if (btn) {
        uint8_t keys[6] = { HID_KEY_A };
        tud_hid_keyboard_report(report_id, 0, keys);
        has_key_kbd = true;
      } else if (has_key_kbd) {
        tud_hid_keyboard_report(report_id, 0, NULL);
        has_key_kbd = false;
      }
      break;
    }

    case REPORT_ID_MOUSE: {
      int8_t dx = 0, dy = 0;

      if (current_mode == REGULAR_MODE) {
        dy -= handle_button(BUTTON_UP, &h_up);
        dy += handle_button(BUTTON_DOWN, &h_down);
        dx -= handle_button(BUTTON_LEFT, &h_left);
        dx += handle_button(BUTTON_RIGHT, &h_right);
      } else {
        double r = 4.0;
        dx = r * sin(2.0 * M_PI * step);
        dy = r * cos(2.0 * M_PI * step);
        step += 0.001;
      }

      tud_hid_mouse_report(report_id, 0x00, dx, dy, 0, 0);
      break;
    }

    case REPORT_ID_CONSUMER_CONTROL: {
      if (btn) {
        uint16_t vol_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(report_id, &vol_down, 2);
        has_key_cons = true;
      } else if (has_key_cons) {
        uint16_t empty = 0;
        tud_hid_report(report_id, &empty, 2);
        has_key_cons = false;
      }
      break;
    }

    case REPORT_ID_GAMEPAD: {
      hid_gamepad_report_t rpt = { .hat = btn ? GAMEPAD_HAT_UP : GAMEPAD_HAT_CENTERED,
                                   .buttons = btn ? GAMEPAD_BUTTON_A : 0 };
      if (btn || has_key_gp) {
        tud_hid_report(report_id, &rpt, sizeof(rpt));
        has_key_gp = btn;
      }
      break;
    }

    default: break;
  }
}

void hid_task(void) {
  static const uint32_t interval = 10;
  static uint32_t last = 0;

  if (board_millis() - last < interval) return;
  last += interval;

  uint32_t btn = board_button_read();
  if (tud_suspended() && btn) tud_remote_wakeup();
  else send_hid_report(REPORT_ID_MOUSE, btn);
}

void tud_hid_report_complete_cb(uint8_t inst, uint8_t const* rpt, uint16_t len) {
  (void)inst; (void)len;
  uint8_t next = rpt[0] + 1;
  if (next < REPORT_ID_COUNT) send_hid_report(next, board_button_read());
}

uint16_t tud_hid_get_report_cb(uint8_t i, uint8_t id, hid_report_type_t t, uint8_t* b, uint16_t l) {
  (void)i; (void)id; (void)t; (void)b; (void)l;
  return 0;
}

void tud_hid_set_report_cb(uint8_t inst, uint8_t id, hid_report_type_t type, uint8_t const* buf, uint16_t size) {
  (void)inst;
  if (type == HID_REPORT_TYPE_OUTPUT && id == REPORT_ID_KEYBOARD && size >= 1) {
    if (buf[0] & KEYBOARD_LED_CAPSLOCK) {
      blink_interval_ms = 0;
      board_led_write(true);
    } else {
      board_led_write(false);
      blink_interval_ms = BLINK_MOUNTED;
    }
  }
}

void led_blinking_task(void) {
  static uint32_t last = 0;
  static bool on = false;

  if (!blink_interval_ms || (board_millis() - last < blink_interval_ms)) return;
  last += blink_interval_ms;
  board_led_write(on);
  on = !on;
}

void gpio_mode_button(uint gpio, uint32_t events) {
  sleep_ms(20);
  if (!gpio_get(BUTTON_MODE)) {
    current_mode = (current_mode == REGULAR_MODE) ? REMOTE_MODE : REGULAR_MODE;
    gpio_put(LED_MODE, current_mode == REMOTE_MODE);
  }
}

int main(void) {
  board_init();
  tud_init(BOARD_TUD_RHPORT);
  if (board_init_after_tusb) board_init_after_tusb();

  button_led_init();
  gpio_set_irq_enabled_with_callback(BUTTON_MODE, GPIO_IRQ_EDGE_FALL, true, &gpio_mode_button);

  while (1) {
    tud_task();
    led_blinking_task();
    hid_task();
  }
}

void tud_mount_cb(void)     { blink_interval_ms = BLINK_MOUNTED; }
void tud_umount_cb(void)    { blink_interval_ms = BLINK_NOT_MOUNTED; }
void tud_suspend_cb(bool _) { blink_interval_ms = BLINK_SUSPENDED; }
void tud_resume_cb(void)    { blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED; }