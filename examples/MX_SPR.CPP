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
#include "../src/SPRITE.CPP"


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

void make_checked_screen(char* buf, int vwidth, int width, int height, char color1, char color2, char color3)
{
    int i, j;
    for (i = 0; i < height / 8; i++) {
        for (j = 0; j < width / 8; j++) {
            if ((i + j) % 2 == 0) {
                draw_bar(buf, vwidth, j * 8, i * 8, j * 8 + 7, i * 8 + 7, color1);
            }
            else {
                draw_bar(buf, vwidth, j * 8, i * 8, j * 8 + 7, i * 8 + 7, color2);
            }
        }
    }
    // draw_box(buf, vwidth, 0, 0, width - 1, height - 1, color3);
}

void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();

    char* pal = load_file("i104x76.pal");
    set_palette_multi((unsigned char *)pal, 0, 256);
    free(pal);

    char *buf = (char *)malloc(320 * 200);
    make_checked_screen(buf, 320, 320, 200, 0x07, 0x08, 0x01);
    send_image_to_vram(_vram, 0, buf, 320, 200);
    free(buf);

    unsigned short img_x = 104, img_y = 76;
    char* img = load_file("i104x76.img");
    // gen image mask
    unsigned char* img_mask = make_image_mask(img, img_x, img_y, 0xFF, 0);

    put_image_to_vram(_vram, 40, 16, img, img_x, img_y);
    put_image_to_vram_mask(_vram, 40 + 140, 16, img_mask, img_x, img_y);
    put_sprite_vram_to_vram(_vram, 40 + 56, 16+96, _vram + 16*_vwidth + 40/4, img_x, img_y, img_mask);
    getch();

    set_screen_mode(0x03);
}
