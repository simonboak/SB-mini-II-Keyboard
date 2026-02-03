#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

// RHPort0 in USB Host mode
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_HOST

// Host configuration
#define CFG_TUH_HUB             1
#define CFG_TUH_HID             4
#define CFG_TUH_MSC             0
#define CFG_TUH_CDC             0
#define CFG_TUH_VENDOR          0

// Max USB device count (hub + downstream devices)
#define CFG_TUH_DEVICE_MAX      (CFG_TUH_HUB ? 4 : 1)

// HID buffer sizes
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_HID_EPIN_BUFSIZE   64
#define CFG_TUH_HID_EPOUT_BUFSIZE  64

#endif
