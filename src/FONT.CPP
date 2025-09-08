#include "mode_x.h"
#include "vga_reg.h"
#include <dos.h>


// 비트마스크 레지스터 설정
void set_bitmask(unsigned char mask) {
    outportb(GC_INDEX, 0x08);
    outportb(GC_DATA, mask);
}


void put_font_to_vram(char *des, unsigned char* font, int x, int y, int width, int height, char color)
{
    set_write_mode(2);

    set_bank_mask(0xF);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 8; j ++) {
            unsigned char m = *font++;
            set_bitmask(m);

            volatile char dummy = des[j];
            des[j] = color;
        }
        des += _vwidth;
    }

    set_write_mode(0);
}
