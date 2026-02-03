# SB Mini II Keyboard Controller

USB keyboard interface for an Apple II replica, using a Raspberry Pi Pico. Converts USB HID keyboard input to 7-bit parallel ASCII output with STROBE signal matching the original Apple II keyboard interface.

**RESET signal is active HIGH vs active LOW** - use a NPN transistor to pull the Apple II's reset line low.

## Pinout

```
                          +---------------+
            D0  <--  GP0  | 1   [USB]   40| VBUS
            D1  <--  GP1  | 2           39| VSYS
                     GND  | 3           38| GND
            D2  <--  GP2  | 4           37| 3V3_EN
            D3  <--  GP3  | 5           36| 3V3
            D4  <--  GP4  | 6           35| ADC_VREF
            D5  <--  GP5  | 7           34| GP28
                     GND  | 8           33| GND
            D6  <--  GP6  | 9           32| GP27
        STROBE  <--  GP7  |10           31| GP26
         RESET  <--  GP8  |11           30| RUN
                     GP9  |12           29| GP22
                     GND  |13           28| GND
                     GP10 |14           27| GP21
                     GP11 |15           26| GP20
                     GP12 |16           25| GP19
                     GP13 |17           24| GP18
                     GND  |18           23| GND
                     GP14 |19           22| GP17
                     GP15 |20           21| GP16
                          +---------------+
```

| Pin     | Function                 | Polarity                   |
|---------|--------------------------|----------------------------|
| GP0-GP6 | Data D0-D6 (7-bit ASCII) | Active high                |
| GP7     | STROBE                   | Active high, ~100us pulse  |
| GP8     | RESET                    | Active high                |
| GP25    | Onboard LED              | On when keyboard connected |

## Features

- USB HID keyboard input via TinyUSB host mode on the Pico's onboard USB port
- Full keycode-to-ASCII conversion with shift, caps lock, and ctrl modifier support
- Arrow keys mapped to Apple II codes (left=0x08, right=0x15, down=0x0A, up=0x0B)
- Ctrl+letter produces control codes 0x01-0x1A
- Ctrl+Print Screen triggers system reset
- Power-on reset pulse on startup
- Onboard LED indicates keyboard connection state

## Hardware Notes

The Pico's USB port operates in host mode. You must supply 5V to VBUS externally to power the connected keyboard (e.g., power the Pico via VSYS and wire 5V to the keyboard's VBUS).

UART stdio is enabled on GP0/GP1 for debug output. These pins are shared with data outputs D0/D1, so debug output will conflict with data output during key presses.

## Building

Requires the Raspberry Pi Pico C/C++ SDK.

```
mkdir build && cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
make
```

This produces `sb_mini_ii_keyboard.uf2`. Hold the BOOTSEL button while connecting the Pico, then copy the UF2 file to the mounted drive.
