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

#define MAX_SCROLL_TICK_SIZE (16)

char* _asset_vram = _vram + 0xB000;

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

// void load_tile_from_image(char *tile_img_filename, int tile_width, int tile_height, char *vram)
// {
//     unsigned char *buf = load_file(tile_img_filename);
//     if (buf == NULL) return;

//     int width = (unsigned)buf[0] | ((unsigned)buf[1] << 8);
//     int height = (unsigned)buf[2] | ((unsigned)buf[3] << 8);

//     unsigned char *buf2 = buf + 4;
//     unsigned char *vram2 = (unsigned char *)vram;
//     for(int i = 0; i < height; i += tile_height) {
//         for(int j = 0; j < width; j += tile_width) {
//             for (int k = 0; k < tile_height; k++) {
//                 send_data_to_vram(vram2, 0, buf2 + k * width, tile_width);
//                 vram2 += tile_width / 4;
//             }
//             buf2 += tile_width;
//         }
//         buf2 += width * (tile_height - 1);
//     }
//     free(buf);

//     _tile_width = tile_width;
//     _tile_height = tile_height;
//     _tile_count = (width / _tile_width) * (height / _tile_height);
// }


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

void clear_bg_mask(unsigned long* bg_mask)
{
    for(int i = 0; i < SCREEN_WIDTH/_tile_width; i ++) {
        bg_mask[i] = 0x0;
    }
}

void do_bg_masking(int sx, int sy, int ex, int ey, unsigned long* bg_mask)
{
    for (int i = sx/_tile_width; i <= ex/_tile_width; i ++) {
        unsigned long mask = bg_mask[i];
        for(int j = sy/_tile_height; j <= ey/_tile_height; j++) {
            mask |= (long)1 << j;
        }
        bg_mask[i] = mask;
    }
}

int _latest_start_map_x;
int _latest_start_pos;

void recover_bg_area_with_bg_mask(unsigned long* bg_mask)
{
    for(int i = 0; i < SCREEN_WIDTH/_tile_width; i ++) {
        unsigned long mask = bg_mask[i];
        if (mask == 0x0) continue;

        long map_idx = _latest_start_map_x + i;
        for(int j = 0; j < SCREEN_HEIGHT/_tile_height; j ++) {
            if (mask & 1) {
                unsigned int tile = _map_data[j*_map_width + map_idx];
                put_vram_to_vram_image( _vram, _latest_start_pos + i * _tile_width, j * _tile_height, _asset_vram + get_image_offs_from_map(tile), _tile_width, _tile_height );
            }
            mask >>= 1;
        }
        bg_mask[i] = 0x0;
    }
}


void draw_char(int start_pos, int x, int y, int width, int height, char *vram, unsigned char *img_mask, int start_map_x, unsigned long *bg_mask)
{
    put_sprite_vram_to_vram(_vram + start_pos/4, x, y, vram, width, height, img_mask);
    do_bg_masking(x, y, x+width-1, y+height-1, bg_mask);
    _latest_start_map_x = start_map_x;
    _latest_start_pos = start_pos;
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

    set_virtual_width(SCREEN_WIDTH * 2 + _tile_width * 2);

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
    int next_pos = start_pos + _tile_width;
    int start_map_x = 0;    
    
    wait_vsync();

    for(int i = 0; i < SCREEN_WIDTH/_tile_width + 1; i++) {
        draw_one_vert_line( _vram, start_pos + i * _tile_width, start_map_x + i );
    }

    int scroll_pos = 0;

    int scroll_speed = 4;
    int char_fling_speed = 4;

    int char_x = 80;
    int char_y = 120;

    unsigned long bg_mask[320/8] = {0, };

    int status = 0;
    int rewind = 0;
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
                    char_x = (char_x > 0) ? char_x - 4 : char_x;
                    break;
                case 0x4D: // right
                    char_x = (char_x < SCREEN_WIDTH - 32) ? char_x + 4 : char_x;
                    break;
                case 0x48: // up
                    char_y = (char_y > 0) ? char_y - 4 : char_y - 0;
                    break;
                case 0x50: // down
                    char_y = (char_y < SCREEN_HEIGHT - 32) ? char_y + 4 : char_y + 0;
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
        scroll_pos += scroll_speed;

        if (scroll_pos >= next_pos) {
            if (start_map_x + SCREEN_WIDTH / _tile_width >= _map_width - 1) {
                status = 2;
                continue;
            }

            if (next_pos + SCREEN_WIDTH >= (SCREEN_WIDTH * 2 + _tile_width * 2)) {
                int delta = scroll_pos - next_pos;
                start_pos = 0;
                scroll_pos = delta;
                rewind = 1;
            } else {
                start_pos += _tile_width;
            }

            next_pos = start_pos + _tile_width;
            start_map_x ++;

            draw_one_vert_line( _vram, start_pos + SCREEN_WIDTH, start_map_x + (SCREEN_WIDTH / _tile_width) );
            if (start_pos >= _tile_width * 2) {
                copy_vram_image_to_vram_image( _vram + (start_pos - _tile_width * 2)/4, _vram + (start_pos + SCREEN_WIDTH)/4, _tile_width, SCREEN_HEIGHT );
            }
        }

        wait_vsync();

        if (!rewind) {
            recover_bg_area_with_bg_mask(bg_mask);
        } else {
            clear_bg_mask(bg_mask);
            rewind = 0;
        }
        
        int x = char_x - start_map_x * _tile_width;
        draw_char(start_pos, x, char_y, char_width, char_height, _asset_vram + 20000/4, msk, start_map_x, bg_mask);

        set_start_addr(scroll_pos / 4);
    }

    if (msk) free(msk);
    if (_map_data) free(_map_data);

    set_screen_mode(0x03);
}
