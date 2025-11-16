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
    if (buf != NULL) fread(buf, len, 1, fp);
    fclose(fp);

    return buf;
}

void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();
    set_virtual_width(320 * 2);

    char *pal = load_file("lych_00.pal");
    if (pal != NULL) {
        set_palette_multi((unsigned char *)pal, 0, 256);
        free(pal);
    }
    char *buf = load_file("lych_01.img");
    if (buf != NULL) {
        send_image_to_vram(_vram, 0, buf+4, 320, 200);
        free(buf);
    }
    buf = load_file("lych_02.img");
    if (buf != NULL) {
        send_image_to_vram(_vram + 320/4, 0, buf+4, 320, 200);
        free(buf);
    }

    int ch = 0;
    int idx = 0;
    while (ch != 27) {
        ch = getch();

        switch (ch) {
        case 75: // left arrow
            if (idx > 0) {
                idx --;
                set_start_addr(idx);
            }
            break;
        case 77: // right arrow
            if (idx < 320/4) {
                idx ++;
                set_start_addr(idx);
            }
            break;
        }
    }

    set_screen_mode(0x03);
}
