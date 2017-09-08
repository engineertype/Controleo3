// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// Allow the setting and clearing of any bit in a 32-bit value

#ifndef BITS_H_
#define BITS_H_

#define SETBIT00        0x00000001
#define SETBIT01        0x00000002
#define SETBIT02        0x00000004
#define SETBIT03        0x00000008
#define SETBIT04        0x00000010
#define SETBIT05        0x00000020
#define SETBIT06        0x00000040
#define SETBIT07        0x00000080
#define SETBIT08        0x00000100
#define SETBIT09        0x00000200
#define SETBIT10        0x00000400
#define SETBIT11        0x00000800
#define SETBIT12        0x00001000
#define SETBIT13        0x00002000
#define SETBIT14        0x00004000
#define SETBIT15        0x00008000
#define SETBIT16        0x00010000
#define SETBIT17        0x00020000
#define SETBIT18        0x00040000
#define SETBIT19        0x00080000
#define SETBIT20        0x00100000
#define SETBIT21        0x00200000
#define SETBIT22        0x00400000
#define SETBIT23        0x00800000
#define SETBIT24        0x01000000
#define SETBIT25        0x02000000
#define SETBIT26        0x04000000
#define SETBIT27        0x08000000
#define SETBIT28        0x10000000
#define SETBIT29        0x20000000
#define SETBIT30        0x40000000
#define SETBIT31        0x80000000

#define CLEARBIT00      0xFFFFFFFE
#define CLEARBIT01      0xFFFFFFFD
#define CLEARBIT02      0xFFFFFFFB
#define CLEARBIT03      0xFFFFFFF7
#define CLEARBIT04      0xFFFFFFEF
#define CLEARBIT05      0xFFFFFFDF
#define CLEARBIT06      0xFFFFFFBF
#define CLEARBIT07      0xFFFFFF7F
#define CLEARBIT08      0xFFFFFEFF
#define CLEARBIT09      0xFFFFFDFF
#define CLEARBIT10      0xFFFFFBFF
#define CLEARBIT11      0xFFFFF7FF
#define CLEARBIT12      0xFFFFEFFF
#define CLEARBIT13      0xFFFFDFFF
#define CLEARBIT14      0xFFFFBFFF
#define CLEARBIT15      0xFFFF7FFF
#define CLEARBIT16      0xFFFEFFFF
#define CLEARBIT17      0xFFFDFFFF
#define CLEARBIT18      0xFFFBFFFF
#define CLEARBIT19      0xFFF7FFFF
#define CLEARBIT20      0xFFEFFFFF
#define CLEARBIT21      0xFFDFFFFF
#define CLEARBIT22      0xFFBFFFFF
#define CLEARBIT23      0xFF7FFFFF
#define CLEARBIT24      0xFEFFFFFF
#define CLEARBIT25      0xFDFFFFFF
#define CLEARBIT26      0xFBFFFFFF
#define CLEARBIT27      0xF7FFFFFF
#define CLEARBIT28      0xEFFFFFFF
#define CLEARBIT29      0xDFFFFFFF
#define CLEARBIT30      0xBFFFFFFF
#define CLEARBIT31      0x7FFFFFFF

#endif // BITS_H_
