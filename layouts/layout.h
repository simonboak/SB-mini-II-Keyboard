/*
 * Keyboard layout table interface.
 *
 * Each layout header defines a pair of LAYOUT_TABLE_SIZE-entry tables indexed
 * by USB HID keycode, plus a keyboard_layout_t describing them. Exactly one
 * layout header is compiled in, chosen by the KEYBOARD_LAYOUT CMake option.
 *
 * To add a layout, copy an existing header, change the table contents and the
 * name, and it becomes selectable as -DKEYBOARD_LAYOUT=<suffix>.
 */

#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdint.h>
#include <stddef.h>     // NULL, for layouts with no AltGr table

// Tables cover HID keycodes 0x00-0x6F. The highest keycode that can produce a
// character is keypad '=' at 0x67; everything above 0x6F is F13-F24 and friends.
#define LAYOUT_TABLE_SIZE 0x70

typedef struct {
    const char *name;       // shown in the UART banner at startup
    const uint8_t *base;    // no modifiers
    const uint8_t *shift;   // Shift held
    const uint8_t *altgr;   // AltGr (right Alt) held; NULL if the layout has none
} keyboard_layout_t;

#endif  // LAYOUT_H
