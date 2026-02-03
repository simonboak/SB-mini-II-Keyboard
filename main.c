/*
 * SB Mini II Keyboard Controller
 *
 * Converts USB keyboard input to 7-bit parallel ASCII output with STROBE,
 * matching the original Apple II keyboard interface.
 *
 * GPIO mapping:
 *   GP0-GP6  - Data bits D0-D6 (7-bit ASCII, active high)
 *   GP7      - STROBE (active high, ~100us pulse on each keypress)
 *   GP8      - RESET  (active low, normally high)
 *   GP25     - Onboard LED (indicates keyboard connected)
 *
 * UART on GP0/GP1 is used for debug output (stdio).
 * Note: GP0/GP1 are shared with D0/D1 data outputs - debug UART
 * will conflict with data output. Disable UART stdio for production use.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "tusb.h"

// ---------------------------------------------------------------------------
// Pin definitions
// ---------------------------------------------------------------------------
#define DATA_PIN_BASE    0      // GP0-GP6
#define DATA_PIN_COUNT   7
#define STROBE_PIN       7      // GP7 - active high
#define RESET_PIN        8      // GP8 - active high
#define LED_PIN          25     // Onboard LED

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
#define STROBE_DURATION_US   100     // ~100us to match original AY-5-3600
#define RESET_DURATION_MS    250     // Power-on reset hold time

// ---------------------------------------------------------------------------
// Apple II arrow key ASCII codes
// ---------------------------------------------------------------------------
#define APPLE_LEFT   0x08   // Ctrl-H
#define APPLE_RIGHT  0x15   // Ctrl-U
#define APPLE_DOWN   0x0A   // Ctrl-J (LF)
#define APPLE_UP     0x0B   // Ctrl-K (VT)

// ---------------------------------------------------------------------------
// HID keycode to ASCII lookup tables
// Indexed by USB HID keycode (0x00 - 0x52)
// ---------------------------------------------------------------------------

// clang-format off
static const uint8_t keycode_to_ascii[] = {
//  0x_0  0x_1  0x_2  0x_3  0x_4  0x_5  0x_6  0x_7  0x_8  0x_9  0x_A  0x_B  0x_C  0x_D  0x_E  0x_F
    0,    0,    0,    0,    'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  // 0x00
    'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2',  // 0x10
    '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0', '\r', 0x1B, 0x7F, '\t',  ' ',  '-',  '=',  '[',  // 0x20
    ']', '\\',   0,   ';', '\'',  '`',  ',',  '.',  '/',   0,    0,    0,    0,    0,    0,    0,    // 0x30
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  0x7F,   0,    0,    0x15, // 0x40
    0x08, 0x0A, 0x0B,                                                                               // 0x50
};

static const uint8_t keycode_to_ascii_shift[] = {
//  0x_0  0x_1  0x_2  0x_3  0x_4  0x_5  0x_6  0x_7  0x_8  0x_9  0x_A  0x_B  0x_C  0x_D  0x_E  0x_F
    0,    0,    0,    0,    'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  // 0x00
    'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '!',  '@',  // 0x10
    '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')', '\r', 0x1B, 0x7F, '\t',  ' ',  '_',  '+',  '{',  // 0x20
    '}',  '|',   0,   ':',  '"',  '~',  '<',  '>',  '?',   0,    0,    0,    0,    0,    0,    0,    // 0x30
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  0x7F,   0,    0,    0x15, // 0x40
    0x08, 0x0A, 0x0B,                                                                               // 0x50
};
// clang-format on

#define KEYCODE_TABLE_SIZE (sizeof(keycode_to_ascii))

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static hid_keyboard_report_t prev_report = {0};
static bool caps_lock = false;

// ---------------------------------------------------------------------------
// GPIO
// ---------------------------------------------------------------------------

static void init_gpio(void) {
    // Data output pins GP0-GP6
    for (int i = 0; i < DATA_PIN_COUNT; i++) {
        gpio_init(DATA_PIN_BASE + i);
        gpio_set_dir(DATA_PIN_BASE + i, GPIO_OUT);
        gpio_put(DATA_PIN_BASE + i, 0);
    }

    // STROBE - active high, idle low
    gpio_init(STROBE_PIN);
    gpio_set_dir(STROBE_PIN, GPIO_OUT);
    gpio_put(STROBE_PIN, 0);

    // RESET - active high, idle low
    gpio_init(RESET_PIN);
    gpio_set_dir(RESET_PIN, GPIO_OUT);
    gpio_put(RESET_PIN, 0);

    // Onboard LED - keyboard connection indicator
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
}

static void pulse_strobe(void) {
    gpio_put(STROBE_PIN, 1);
    sleep_us(STROBE_DURATION_US);
    gpio_put(STROBE_PIN, 0);
}

static void pulse_reset(void) {
    gpio_put(RESET_PIN, 1);
    sleep_ms(RESET_DURATION_MS);
    gpio_put(RESET_PIN, 0);
}

static void output_key(uint8_t ascii) {
    // Set 7-bit ASCII value on GP0-GP6
    for (int i = 0; i < DATA_PIN_COUNT; i++) {
        gpio_put(DATA_PIN_BASE + i, (ascii >> i) & 1);
    }
    pulse_strobe();
}

// ---------------------------------------------------------------------------
// Keycode conversion
// ---------------------------------------------------------------------------

static uint8_t hid_to_ascii(uint8_t keycode, uint8_t modifier) {
    if (keycode >= KEYCODE_TABLE_SIZE) {
        return 0;
    }

    bool shift = (modifier & (KEYBOARD_MODIFIER_LEFTSHIFT |
                              KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
    bool ctrl  = (modifier & (KEYBOARD_MODIFIER_LEFTCTRL |
                              KEYBOARD_MODIFIER_RIGHTCTRL)) != 0;

    // Caps Lock inverts shift for letters only
    bool is_letter = (keycode >= HID_KEY_A && keycode <= HID_KEY_Z);
    if (caps_lock && is_letter) {
        shift = !shift;
    }

    uint8_t ascii = shift ? keycode_to_ascii_shift[keycode]
                          : keycode_to_ascii[keycode];

    // Ctrl + letter: produce 0x01 (Ctrl-A) through 0x1A (Ctrl-Z)
    if (ctrl && is_letter) {
        ascii = (keycode - HID_KEY_A) + 1;
    }

    return ascii;
}

// ---------------------------------------------------------------------------
// HID report processing
// ---------------------------------------------------------------------------

static bool is_new_key(uint8_t keycode, const hid_keyboard_report_t *prev) {
    for (int i = 0; i < 6; i++) {
        if (prev->keycode[i] == keycode) {
            return false;
        }
    }
    return true;
}

static void process_kbd_report(hid_keyboard_report_t const *report) {
    // Toggle Caps Lock on new press
    for (int i = 0; i < 6; i++) {
        if (report->keycode[i] == HID_KEY_CAPS_LOCK &&
            is_new_key(HID_KEY_CAPS_LOCK, &prev_report)) {
            caps_lock = !caps_lock;
        }
    }

    // Process new keypresses
    for (int i = 0; i < 6; i++) {
        uint8_t keycode = report->keycode[i];
        if (keycode == 0) {
            continue;
        }
        if (!is_new_key(keycode, &prev_report)) {
            continue;
        }

        // Ctrl + Print Screen = system reset
        if (keycode == HID_KEY_PRINT_SCREEN &&
            (report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL |
                                 KEYBOARD_MODIFIER_RIGHTCTRL))) {
            printf("RESET triggered (Ctrl+PrtSc)\n");
            pulse_reset();
            continue;
        }

        uint8_t ascii = hid_to_ascii(keycode, report->modifier);
        if (ascii) {
            printf("Key: 0x%02X\n", ascii);
            output_key(ascii);
        }
    }

    prev_report = *report;
}

// ---------------------------------------------------------------------------
// TinyUSB Host HID callbacks
// ---------------------------------------------------------------------------

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        printf("Keyboard connected (dev=%d, instance=%d)\n", dev_addr, instance);
        gpio_put(LED_PIN, 1);

        // Request boot protocol for fixed-format reports
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            printf("Error: failed to request HID report\n");
        }
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
    printf("Keyboard disconnected\n");
    gpio_put(LED_PIN, 0);
    memset(&prev_report, 0, sizeof(prev_report));
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const *report, uint16_t len) {
    if (tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_KEYBOARD) {
        if (len >= sizeof(hid_keyboard_report_t)) {
            process_kbd_report((hid_keyboard_report_t const *)report);
        }
    }

    // Continue receiving reports
    tuh_hid_receive_report(dev_addr, instance);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(void) {
    stdio_init_all();
    init_gpio();

    printf("SB Mini II Keyboard Controller\n");

    // Power-on reset pulse
    printf("Power-on reset...\n");
    pulse_reset();

    // Initialize TinyUSB host
    tusb_init();

    printf("Waiting for keyboard...\n");

    while (true) {
        tuh_task();
    }

    return 0;
}
