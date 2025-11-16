#ifndef __MODE_X_H_
#include "mode_x.h"
#endif
#ifndef __VGA_REG_H_
#include "vga_reg.h"
#endif
#include <dos.h>


// 특정 팔레트 색 설정
void set_palette(int idx, unsigned char red, unsigned char green, unsigned char blue)
{
    outportb(DAC_INDEX, idx);
    outportb(DAC_DATA, red);
    outportb(DAC_DATA, green);
    outportb(DAC_DATA, blue);
}


// 복수개 팔레트 설정
void set_palette_multi(unsigned char* pal, int start, int count)
{
    outportb(DAC_INDEX, start);
    for (int i = 0; i < count; i++) {
        outportb(DAC_DATA, *pal++);
        outportb(DAC_DATA, *pal++);
        outportb(DAC_DATA, *pal++);
    }
}

