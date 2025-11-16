#ifndef __MODE_X_H_
#include "mode_x.h"
#endif
#ifndef __VGA_REG_H_
#include "vga_reg.h"
#endif
#include <dos.h>


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
