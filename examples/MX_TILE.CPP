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

    size_t len = filelength(fileno(fp));
    char *buf = (char *)malloc(len);
    if (buf != NULL) fread(buf, len, 1, fp);
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

    char *pal = load_file("tile.pal");
    if (pal != NULL) {
        set_palette_multi((unsigned char *)pal, 0, 256);
        free(pal);
    }
    char *buf = load_file("tile.img");
    if (buf != NULL) {
        send_image_to_vram(_vram + 0xB000, 0, buf+4, 40*2, 40);
        free(buf);
    }

    int idx0 = 0;
    for(int i = 0; i < screen_height; i += 40) {
        int idx = idx0;
        idx0 ^= 1;
        for(int j = 0; j < 320; j += 40) {
            put_vram_to_vram( _vram, _vram + 0xB000 + idx * 40/4, j, i, 40, 40 );
            idx ^= 1;
        }
    }

    getch();

    set_screen_mode(0x03);
}
