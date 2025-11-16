#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <io.h>
#include "../src/MODE_X.H"
#include "../src/VGA_REG.H"

#include "../src/VGA_CTL.CPP"
#include "../src/PALETTE.CPP"
#include "../src/GDI.CPP"
#include "../src/IMAGE.CPP"

// #define LINES_240

#define SCREEN_WIDTH    (320)
#ifdef LINES_240
#define SCREEN_HEIGHT   (240)
#else
#define SCREEN_HEIGHT   (200)
#endif


char *load_file(char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return NULL;

    size_t len = (size_t)filelength(fileno(fp));
    char *buf = (char *)malloc(len);
    fread(buf, len, 1, fp);
    fclose(fp);

    return buf;
}

void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();
 
    #ifdef LINES_240
    set_crtc_to_240_lines();
    int screen_height = 240;
    #else
    int screen_height = 200;
    #endif
    set_virtual_width(320 * 2);

    char *pal = load_file("ep1_00.pal");
    set_palette_multi((unsigned char *)pal, 0, 256);
    free(pal);

    char *buf = load_file("ep1_01.img");
    send_image_to_vram(_vram, 0, buf+4, 320, 200);
    send_image_to_vram(_vram + 320/4, 0, buf+4, 320, 200);
    free(buf);

    buf = load_file("ep1_02.img");
    send_image_to_vram(_vram + 0x3E80*2, 0, buf+4, 320, 200);
    send_image_to_vram(_vram + 0x3E80*2 + 320/4, 0, buf+4, 320, 200);
    free(buf);

    int ch = 0;
    int x = 0;
    int y = 0;
    while (ch != 27) {
        ch = getch();

        switch (ch) {
        case 72: // up arrow
            if (y > 0) {
                y --;
                set_start_addr(x + y * 320U/4*2);
            }
            break;
        case 80: // down arrow
            if (y < 400-screen_height-1) {
                y ++;
                set_start_addr(x + y * 320U/4*2);
            }
            break;
        case 75: // left arrow
            if (x > 0) {
                x --;
                set_start_addr(x + y * 320U/4*2);
            }
            break;
        case 77: // right arrow
            if (x < 320/4-1) {
                x ++;
                set_start_addr(x + y * 320U/4*2);
            }
            break;
        }

    }

    set_screen_mode(0x03);
}
