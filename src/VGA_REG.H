#ifndef __VGA_REG_H_
#define __VGA_REG_H_


#define GC_INDEX   0x3CE
#define GC_DATA    0x3CF
#define   GC_SET_RESET              0x00
#define   GC_ENABLE_SET_RESET       0x01
#define   GC_FUNC                   0x03
#define   GC_READ_MAP_SELECT        0x04
#define   GC_MODE                   0x05
#define   GC_BITMASK                0x08

#define SA_INDEX   0x3C4
#define SA_DATA    0x3C5
#define   SA_PLANE_MASK             0x02
#define   SA_MEMORY_MODE            0x04

#define DAC_INDEX  0x3C8
#define DAC_DATA   0x3C9

#define CRTC_INDEX 0x3D4
#define CRTC_DATA  0x3D5
#define   CRTC_START_ADDR_HI        0x0C
#define   CRTC_START_ADDR_LO        0x0D
#define   CRTC_VERTICAL_SYNC_END    0x11
#define   CRTC_VERT_DISP_ENABLE_END 0x12
#define   CRTC_OFFSET               0x13
#define   CRTC_UNDERLINE_LOCATTION  0x14
#define   CRTC_MODE_CONTROL         0x17

#define MISC_OUTPUT 0x03C2


#endif // __VGA_REG_H_
