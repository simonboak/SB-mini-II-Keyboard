/*
 * US English (ANSI) keyboard layout.
 */

#ifndef LAYOUT_US_H
#define LAYOUT_US_H

#include "layouts/layout.h"

// clang-format off
static const uint8_t keycode_to_ascii_us[LAYOUT_TABLE_SIZE] = {
//  0x_0  0x_1  0x_2  0x_3  0x_4  0x_5  0x_6  0x_7  0x_8  0x_9  0x_A  0x_B  0x_C  0x_D  0x_E  0x_F
    0,    0,    0,    0,    'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  // 0x00
    'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '1',  '2',  // 0x10
    '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0', '\r',  0x1B, 0x08, '\t', ' ',  '-',  '=',  '[',  // 0x20
    ']',  '\\', 0,    ';',  '\'', '`',  ',',  '.',  '/',  0,    0,    0,    0,    0,    0,    0,    // 0x30
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0x7F, 0,    0,    0x15, // 0x40
    0x08, 0x0A, 0x0B, 0,    '/',  '*',  '-',  '+',  '\r', '1',  '2',  '3',  '4',  '5',  '6',  '7',  // 0x50
    '8',  '9',  '0',  '.',  0,    0,    0,    '=',  0,    0,    0,    0,    0,    0,    0,    0,    // 0x60
};

static const uint8_t keycode_to_ascii_us_shift[LAYOUT_TABLE_SIZE] = {
//  0x_0  0x_1  0x_2  0x_3  0x_4  0x_5  0x_6  0x_7  0x_8  0x_9  0x_A  0x_B  0x_C  0x_D  0x_E  0x_F
    0,    0,    0,    0,    'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  // 0x00
    'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '!',  '@',  // 0x10
    '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '\r', 0x1B, 0x08, '\t', ' ',  '_',  '+',  '{',  // 0x20
    '}',  '|',  0,    ':',  '"',  '~',  '<',  '>',  '?',  0,    0,    0,    0,    0,    0,    0,    // 0x30
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0x7F, 0,    0,    0x15, // 0x40
    0x08, 0x0A, 0x0B, 0,    '/',  '*',  '-',  '+', '\r',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  // 0x50
    '8',  '9',  '0',  '.',  0,    0,    0,    '=',  0,    0,    0,    0,    0,    0,    0,    0,    // 0x60
};
// clang-format on

_Static_assert(sizeof(keycode_to_ascii_us) == LAYOUT_TABLE_SIZE,
               "US base table is not LAYOUT_TABLE_SIZE entries");
_Static_assert(sizeof(keycode_to_ascii_us_shift) == LAYOUT_TABLE_SIZE,
               "US shift table is not LAYOUT_TABLE_SIZE entries");

static const keyboard_layout_t keyboard_layout = {
    .name  = "US",
    .base  = keycode_to_ascii_us,
    .shift = keycode_to_ascii_us_shift,
    .altgr = NULL,
};

#endif  // LAYOUT_US_H
