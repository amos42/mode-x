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


#define TILE_WIDTH  40
#define TILE_HEIGHT 40
#define MAX_SCREEN_HEIGHT  (SCREEN_HEIGHT * 2 + TILE_HEIGHT)

char* _asset_vram = (char*)0xA0000000 + 0xB000;


void draw_one_line(int line_y, int start_idx)
{
    for(int i = 0; i < SCREEN_WIDTH; i += TILE_WIDTH) {
        put_vram_to_vram( _vram, _asset_vram + start_idx * TILE_WIDTH/4, i, line_y, TILE_WIDTH, TILE_HEIGHT );
        start_idx ^= 1;
    }
}


void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();

    #ifdef LINES_240
    set_crtc_to_240_lines();
    #endif

    char *pal = load_file("tile.pal");
    if (pal != NULL) {
        set_palette_multi((unsigned char *)pal, 0, 256);
        free(pal);
    }
    char *buf = load_file("tile.img");
    if (buf != NULL) {
        send_image_to_vram(_asset_vram, 0, buf+4, 40*2, 40);
        free(buf);
    }

    long idx = MAX_SCREEN_HEIGHT - SCREEN_HEIGHT;

    int tile_idx = 0;
    for(int i = SCREEN_HEIGHT - TILE_HEIGHT; i >= 0; i -= TILE_HEIGHT) {
        draw_one_line( idx + i, tile_idx );
        tile_idx ^= 1;
    }

    set_start_addr(SCREEN_WIDTH/4 * idx);

    long bound_idx = idx;
    int speed = 1;

    int status = 0;
    while (status != 2) {
        if ( kbhit() ){
            int ch = getch();
            switch (ch) {
                case 0x20: // space
                    status = !status;
                    break;
                case 0x1B: // ESC
                    status = 2;
                    break;
                case '+':
                    if(speed < TILE_HEIGHT/2) speed++;
                    break;
                case '-':
                    if(speed > 1) speed--;
                    break;
            }
        }

        if (status != 0) {
            delay(100);
            continue;
        }

        idx -= speed;
        if (idx < bound_idx) {
            if (bound_idx + TILE_HEIGHT < 0) {
                bound_idx = MAX_SCREEN_HEIGHT - SCREEN_HEIGHT;
                idx = MAX_SCREEN_HEIGHT - SCREEN_HEIGHT - speed;
            }
            draw_one_line( bound_idx - TILE_HEIGHT, tile_idx );
            tile_idx ^= 1;
            if (bound_idx + SCREEN_HEIGHT + TILE_HEIGHT < MAX_SCREEN_HEIGHT) {
                copy_vram_image_to_vram_image( _vram + (bound_idx + SCREEN_HEIGHT)*SCREEN_WIDTH/4, _vram + (bound_idx - TILE_HEIGHT)*SCREEN_WIDTH/4, SCREEN_WIDTH, TILE_HEIGHT );
            }
            bound_idx -= TILE_HEIGHT;
        }

        wait_vsync();

        set_start_addr(SCREEN_WIDTH/4 * idx);
    }

    set_screen_mode(0x03);
}
