/*
 * SB Mini II Keyboard Controller
 *
 * Converts USB keyboard input to 7-bit parallel ASCII output with STROBE,
 * matching the original Apple II keyboard interface.
 *
 * GPIO mapping:
 *   GP0      - UART TX (debug output, 115200 baud)
 *   GP1      - UART RX
 *   GP2-GP8  - Data bits D0-D6 (7-bit ASCII, active high)
 *   GP9      - STROBE (active high, ~100us pulse on each keypress)
 *   GP10     - RESET  (active high)
 *   GP11     - SHIFT  (high when Shift key held, active high)
 *   GP25     - Onboard LED (blinks while searching, solid when connected)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "tusb.h"

#include "layouts/layout.h"

// Layout is chosen at build time: cmake -DKEYBOARD_LAYOUT=de ..
#ifndef KEYBOARD_LAYOUT_HEADER
#define KEYBOARD_LAYOUT_HEADER "layouts/layout_us.h"
#endif
#include KEYBOARD_LAYOUT_HEADER

// ---------------------------------------------------------------------------
// Pin definitions
// ---------------------------------------------------------------------------
#define DATA_PIN_BASE    2      // GP2-GP8
#define DATA_PIN_COUNT   7
#define STROBE_PIN       9      // GP9 - active high
#define RESET_PIN        10     // GP10 - active high
#define SHIFT_PIN        11     // GP11 - high when Shift held
#define LED_PIN          25     // Onboard LED

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
#define STROBE_DURATION_US   100     // ~100us to match original AY-5-3600
#define RESET_DURATION_MS    250     // Power-on reset hold time
#define LED_BLINK_MS         500     // LED blink half-period while searching

// ---------------------------------------------------------------------------
// Apple II arrow key ASCII codes
// ---------------------------------------------------------------------------
#define APPLE_LEFT   0x08   // Ctrl-H
#define APPLE_RIGHT  0x15   // Ctrl-U
#define APPLE_DOWN   0x0A   // Ctrl-J (LF)
#define APPLE_UP     0x0B   // Ctrl-K (VT)

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

// The compiled-in layout. A pointer rather than direct use of keyboard_layout
// so that runtime layout switching stays a small change later.
static const keyboard_layout_t *layout = &keyboard_layout;
static hid_keyboard_report_t prev_report = {0};
static bool caps_lock = true;   // Apple II expects uppercase; on by default
static bool kbd_connected = false;

// ---------------------------------------------------------------------------
// Keyboard lock LEDs
//
// A USB keyboard does not light its own Caps Lock LED. As the host we have to
// send it an output report saying which lock LEDs to light, so the LED tracks
// caps_lock rather than the physical key.
// ---------------------------------------------------------------------------
static uint8_t kbd_dev_addr = 0;
static uint8_t kbd_instance = 0;
static uint8_t kbd_led_report = 0;      // static: the transfer is async, so this
                                        // buffer must outlive the call below
static bool led_update_pending = false;
static bool led_report_busy = false;

// ---------------------------------------------------------------------------
// GPIO
// ---------------------------------------------------------------------------

static void init_gpio(void) {
    // Data output pins GP2-GP8
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

    // SHIFT - high when Shift key held, for Apple II game connector
    gpio_init(SHIFT_PIN);
    gpio_set_dir(SHIFT_PIN, GPIO_OUT);
    gpio_put(SHIFT_PIN, 0);

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
    // Set 7-bit ASCII value on GP2-GP8
    for (int i = 0; i < DATA_PIN_COUNT; i++) {
        gpio_put(DATA_PIN_BASE + i, (ascii >> i) & 1);
    }
    pulse_strobe();
}

// ---------------------------------------------------------------------------
// Keycode conversion
// ---------------------------------------------------------------------------

static uint8_t hid_to_ascii(uint8_t keycode, uint8_t modifier) {
    if (keycode >= LAYOUT_TABLE_SIZE) {
        return 0;
    }

    bool shift = (modifier & (KEYBOARD_MODIFIER_LEFTSHIFT |
                              KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
    bool ctrl  = (modifier & (KEYBOARD_MODIFIER_LEFTCTRL |
                              KEYBOARD_MODIFIER_RIGHTCTRL)) != 0;
    bool altgr = (modifier & KEYBOARD_MODIFIER_RIGHTALT) != 0;

    // Which key is a letter comes from the layout, not the keycode: German
    // transposes Y and Z, so the keycode alone says nothing about the letter.
    uint8_t unshifted = layout->base[keycode];
    bool is_letter = (unshifted >= 'a' && unshifted <= 'z');

    // Caps Lock inverts shift for letters only, so digits stay digits
    if (caps_lock && is_letter) {
        shift = !shift;
    }

    uint8_t ascii;
    if (altgr && layout->altgr) {
        ascii = layout->altgr[keycode];
    } else if (shift) {
        ascii = layout->shift[keycode];
    } else {
        ascii = unshifted;
    }

    // Ctrl + letter: produce 0x01 (Ctrl-A) through 0x1A (Ctrl-Z). Derived from
    // the letter the layout produces, so Ctrl-Z is the key labelled Z.
    if (ctrl && is_letter) {
        ascii = (unshifted - 'a') + 1;
    }

    return ascii;
}

// ---------------------------------------------------------------------------
// Keyboard LED output report
// ---------------------------------------------------------------------------

// Called from the main loop rather than straight from a USB callback: this is a
// control transfer, and issuing one from inside mount/report callbacks races
// with the transfers TinyUSB is already running there.
static void update_kbd_leds(void) {
    if (!kbd_connected || !led_update_pending || led_report_busy) {
        return;
    }

    kbd_led_report = caps_lock ? KEYBOARD_LED_CAPSLOCK : 0;

    if (tuh_hid_set_report(kbd_dev_addr, kbd_instance, 0, HID_REPORT_TYPE_OUTPUT,
                           &kbd_led_report, sizeof(kbd_led_report))) {
        led_update_pending = false;
        led_report_busy = true;
    }
    // If the call failed the keyboard is busy; the pending flag stays set and
    // we retry on the next pass.
}

void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance,
                                    uint8_t report_id, uint8_t report_type,
                                    uint16_t len) {
    (void)dev_addr;
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)len;
    led_report_busy = false;
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
    // Output Shift state on GP11 for Apple II game connector
    bool shift_held = (report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT |
                                           KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
    gpio_put(SHIFT_PIN, shift_held);

    // Toggle Caps Lock on new press
    for (int i = 0; i < 6; i++) {
        if (report->keycode[i] == HID_KEY_CAPS_LOCK &&
            is_new_key(HID_KEY_CAPS_LOCK, &prev_report)) {
            caps_lock = !caps_lock;
            led_update_pending = true;
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
        kbd_connected = true;
        gpio_put(LED_PIN, 1);

        // Push the current caps_lock state to the keyboard's LED, so a freshly
        // plugged keyboard shows the boot-on state instead of a dark LED
        kbd_dev_addr = dev_addr;
        kbd_instance = instance;
        led_report_busy = false;
        led_update_pending = true;

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
    kbd_connected = false;
    led_update_pending = false;
    led_report_busy = false;
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
    printf("Layout: %s\n", layout->name);

    // Power-on reset pulse
    printf("Power-on reset...\n");
    pulse_reset();

    // Initialize TinyUSB host
    tusb_init();

    printf("Waiting for keyboard...\n");

    while (true) {
        tuh_task();
        update_kbd_leds();

        // Blink LED while waiting for keyboard; solid on when connected
        if (!kbd_connected) {
            uint32_t t = to_ms_since_boot(get_absolute_time());
            gpio_put(LED_PIN, (t / LED_BLINK_MS) % 2);
        }
    }

    return 0;
}
