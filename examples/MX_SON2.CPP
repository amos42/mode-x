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

unsigned char *load_file(char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return NULL;

    size_t len = filelength(fileno(fp));
    char *buf = (char *)malloc(len);
    if (buf != NULL) fread(buf, len, 1, fp);
    fclose(fp);

    return buf;
}

#ifdef LINES_240
#define ONE_PAGE_SIZE (SCREEN_WIDTH/4*240)
#else
#define ONE_PAGE_SIZE (SCREEN_WIDTH/4*208)
#endif

#define MAX_SCROLL_TICK_SIZE (16)

char* _asset_vram = (char*)0xA0000000 + ONE_PAGE_SIZE*3;

int _tile_width = 0;
int _tile_height = 0;
int _tile_count = 0;
int _map_width = 0;
int _map_height = 0;

unsigned short *_map_data = NULL;

void load_map(char *map_filename)
{
    unsigned char *buf = load_file(map_filename);
    if (buf == NULL) return;

    _map_width = (unsigned)buf[0] | ((unsigned)buf[1] << 8);
    _map_height = (unsigned)buf[4] | ((unsigned)buf[5] << 8);
    _map_data = (unsigned short *)malloc(_map_width * _map_height * sizeof(short));
    memcpy(_map_data, buf+8, _map_width * _map_height * sizeof(short));
    free(buf);
}

int load_tile(char *tile_filename, char *vram)
{
    unsigned char *buf = load_file(tile_filename);
    if (buf == NULL) return 0;
    _tile_width = (unsigned)buf[0] | ((unsigned)buf[1] << 8);
    _tile_height = (unsigned)buf[4] | ((unsigned)buf[5] << 8);
    _tile_count = (unsigned)buf[8] | ((unsigned)buf[9] << 8);
    send_data_to_vram(vram, 0, buf + 16, _tile_width * _tile_height * _tile_count);
    free(buf);

    return _tile_width * _tile_height * _tile_count;
}

unsigned int get_image_offs_from_map(unsigned int idx)
{
    return idx * _tile_width / 4 * _tile_height;
}

void draw_one_vert_line(char *vram, int line_x, int map_x)
{
    int idx = map_x;
    for (int y = 0; y < _map_height; y++) {
        unsigned int tile = _map_data[y*_map_width + map_x];
        put_vram_to_vram_image( vram, line_x, y * _tile_height, _asset_vram + get_image_offs_from_map(tile), _tile_width, _tile_height );
        idx += _map_width;
    }
}

void draw_char(char *tvram, int x, int y, int width, int height, char *vram, unsigned char *img_mask)
{
    put_sprite_vram_to_vram(tvram, x, y, vram, width, height, img_mask);
}


void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();

    #ifdef LINES_240
    set_crtc_to_240_lines();
    #endif

    char *pal = load_file("son_til.pal");
    if (pal != NULL) {
        set_palette_multi((unsigned char *)pal, 0, 256);
        free(pal);
    }
    pal = load_file("sonic.pal");
    if (pal != NULL) {
        set_palette_multi((unsigned char *)pal + 128*3, 128, 256-128);
        free(pal);
    }

    load_tile("son_tile.til", _asset_vram);
    // load_tile_from_image("son_til.img", 8, 8, _asset_vram);
    load_map("son_map.map");

    char *buf = load_file("sonic.img");
    int char_width, char_height;
    unsigned char *msk;
    if (buf != NULL) {
        char_width = *(short *)buf;
        char_height = *(short *)(buf + 2);
        send_image_to_vram(_asset_vram + 20000/4, 0, buf+4, char_width, char_height);
        msk = make_image_mask(buf+4, 0, char_width, char_height, 0xFF, 0);
        free(buf);
    }

    int start_pos = 0;

    int scroll_speed = 4;
    int char_fling_speed = 4;

    int char_x = 80;
    int char_y = 120;

    int status = 0;
    int page = 0;
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
                case 0x4B: // left
                    char_x = (char_x - start_pos > 0) ? char_x - 4 : char_x;
                    break;
                case 0x4D: // right
                    char_x = (char_x - start_pos < SCREEN_WIDTH - char_width) ? char_x + 4 : char_x;
                    break;
                case 0x48: // up
                    char_y = (char_y > 0) ? char_y - 4 : char_y - 0;
                    break;
                case 0x50: // down
                    char_y = (char_y < SCREEN_HEIGHT - char_height) ? char_y + 4 : char_y + 0;
                    break;
                case '+':
                    if(scroll_speed < MAX_SCROLL_TICK_SIZE) scroll_speed+=4;
                    break;
                case '-':
                    if(scroll_speed > 4) scroll_speed-=4;
                    break;
            }
        }
        if (status != 0) {
            continue;
        }

        char_x += char_fling_speed;
        start_pos += scroll_speed;

        wait_vsync();

        int start_map_x = start_pos / 8;
        for(int i = 0; i < SCREEN_WIDTH/_tile_width; i++) {
            draw_one_vert_line( _vram + page * ONE_PAGE_SIZE, i * _tile_width, start_map_x + i );
        }

        int x = char_x - start_pos;
        draw_char( _vram + page * ONE_PAGE_SIZE, x, char_y, char_width, char_height, _asset_vram + 20000/4, msk );

        set_start_addr(page * ONE_PAGE_SIZE);
        page = (page + 1) % 3;

        if (start_map_x + SCREEN_WIDTH / _tile_width >= _map_width - 1) {
            status = 2;
        }
    }

    if (msk) free(msk);
    if (_map_data) free(_map_data);

    set_screen_mode(0x03);
}
