/*
 * German (DIN 2137 T1) keyboard layout.
 *
 * The Apple II charset is 7-bit uppercase ASCII, so the German umlaut and eszett
 * keys have nothing they can legitimately emit. Rather than leave them dead,
 * this layout reuses those slots for the characters that a German keyboard
 * normally reaches via AltGr, which the Apple II does need. The scheme is
 * consistently "plain bracket unshifted, curly/bar shifted":
 *
 *   Shift+3 (would be §)  ->  @
 *   ß  ->  ~        Ü  ->  ] / }
 *   Ä  ->  [ / {    Ö  ->  \ / |
 *
 * With those substitutions every character in 0x20-0x5F is reachable without an
 * AltGr layer, so .altgr is NULL. Note Y/Z are transposed at 0x1C/0x1D, and the
 * '#' key is mapped at both 0x31 and 0x32 because ISO keyboards differ over
 * which of the two they report.
 *
 * Character tables contributed and tested by a German user; keypad rows
 * (0x54-0x67) are layout-independent and taken from the US layout.
 */

#ifndef LAYOUT_DE_H
#define LAYOUT_DE_H

#include "layouts/layout.h"

// clang-format off
static const uint8_t keycode_to_ascii_de[LAYOUT_TABLE_SIZE] = {
//  0x_0  0x_1  0x_2  0x_3  0x_4  0x_5  0x_6  0x_7  0x_8  0x_9  0x_A  0x_B  0x_C  0x_D  0x_E  0x_F
    0,    0,    0,    0,    'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  // 0x00
    'm',  'n',  'o',  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'z',  'y',  '1',  '2',  // 0x10
    '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0', '\r',  0x1B, 0x08, '\t', ' ',  '~',  0,    ']',  // 0x20
    '+',  '#',  '#',  '\\', '[',  '^',  ',',  '.',  '-',  0,    0,    0,    0,    0,    0,    0,    // 0x30
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0x7F, 0,    0,    0x15, // 0x40
    0x08, 0x0A, 0x0B, 0,    '/',  '*',  '-',  '+',  '\r', '1',  '2',  '3',  '4',  '5',  '6',  '7',  // 0x50
    '8',  '9',  '0',  '.',  '<',  0,    0,    '=',  0,    0,    0,    0,    0,    0,    0,    0,    // 0x60
};

static const uint8_t keycode_to_ascii_de_shift[LAYOUT_TABLE_SIZE] = {
//  0x_0  0x_1  0x_2  0x_3  0x_4  0x_5  0x_6  0x_7  0x_8  0x_9  0x_A  0x_B  0x_C  0x_D  0x_E  0x_F
    0,    0,    0,    0,    'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  // 0x00
    'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Z',  'Y',  '!',  '"',  // 0x10
    '@',  '$',  '%',  '&',  '/',  '(',  ')',  '=', '\r',  0x1B, 0x08, '\t', ' ',  '?',  '`',  '}',  // 0x20
    '*',  '\'', '\'', '|',  '{',  0,    ';',  ':',  '_',  0,    0,    0,    0,    0,    0,    0,    // 0x30
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0x7F, 0,    0,    0x15, // 0x40
    0x08, 0x0A, 0x0B, 0,    '/',  '*',  '-',  '+', '\r',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  // 0x50
    '8',  '9',  '0',  '.',  '>',  0,    0,    '=',  0,    0,    0,    0,    0,    0,    0,    0,    // 0x60
};
// clang-format on

_Static_assert(sizeof(keycode_to_ascii_de) == LAYOUT_TABLE_SIZE,
               "German base table is not LAYOUT_TABLE_SIZE entries");
_Static_assert(sizeof(keycode_to_ascii_de_shift) == LAYOUT_TABLE_SIZE,
               "German shift table is not LAYOUT_TABLE_SIZE entries");

static const keyboard_layout_t keyboard_layout = {
    .name  = "German (T1)",
    .base  = keycode_to_ascii_de,
    .shift = keycode_to_ascii_de_shift,
    .altgr = NULL,
};

#endif  // LAYOUT_DE_H
