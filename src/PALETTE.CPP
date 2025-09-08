#include "mode_x.h"
#include "vga_reg.h"
#include <dos.h>


// 특정 팔레트 색 설정
void set_pallete(int idx, unsigned char red, unsigned char green, unsigned char blue)
{
    outportb(DAC_INDEX, idx);
    outportb(DAC_DATA, red);
    outportb(DAC_DATA, green);
    outportb(DAC_DATA, blue);
}


// 전체 팔레트 설정
void set_pallete_all(unsigned char* pal)
{
    outportb(DAC_INDEX, 0);
    for (int i = 0; i < 256; i++) {
        outportb(DAC_DATA, *pal++);
        outportb(DAC_DATA, *pal++);
        outportb(DAC_DATA, *pal++);
    }
}

