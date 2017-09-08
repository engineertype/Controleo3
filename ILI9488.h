// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// Registers for the ILI9488 controller


// Registers
#define ILI9488_SOFTRESET           0x01
#define ILI9488_READ_DISPLAY_INFO   0x04
#define ILI9488_SLEEPIN             0x10
#define ILI9488_SLEEPOUT            0x11
#define ILI9488_NORMAL              0x13
#define ILI9488_INVERTOFF           0x20
#define ILI9488_INVERTON            0x21
#define ILI9488_PIXELSOFF           0x22
#define ILI9488_PIXELSON            0x23
#define ILI9488_DISPLAYOFF          0x28
#define ILI9488_DISPLAYON           0x29
#define ILI9488_COLADDRSET          0x2A
#define ILI9488_PAGEADDRSET         0x2B
#define ILI9488_MEMORYWRITE         0x2C
#define ILI9488_MEMORYREAD          0x2E
#define ILI9488_MADCTL              0x36
#define ILI9488_PIXELFORMAT         0x3A

#define ILI9488_MADCTL_MY  			0x80
#define ILI9488_MADCTL_MX           0x40
#define ILI9488_MADCTL_MV           0x20
#define ILI9488_MADCTL_RGB          0x00
#define ILI9488_MADCTL_MH           0x04
#define ILI9488_MADCTL_BGR          0x08
#define ILI9488_MADCTL_ML           0x10
