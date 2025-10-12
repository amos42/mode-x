#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <io.h>
#include "vga_reg.h"


// #define LINES_240

#define SCREEN_WIDTH    (320)
#ifdef LINES_240
#define SCREEN_HEIGHT   (240)
#else
#define SCREEN_HEIGHT   (200)
#endif

char* _vram = (char*)0xA0000000;
char* _asset_vram = (char*)0xA0000000 + 0xB000;
int _vwidth = SCREEN_WIDTH / 4;
int _vheight = SCREEN_HEIGHT;


void set_screen_mode(int mode)
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}

void turn_off_chain4()
{
    outportb(SA_INDEX, SA_MEMORY_MODE);
    // outportb(SA_DATA, inportb(SA_DATA) & ~8);
    outportb(SA_DATA, 0x06);

    outportb(CRTC_INDEX, CRTC_UNDERLINE_LOCATTION);
    outportb(CRTC_DATA, 0x00);
    outportb(CRTC_INDEX, CRTC_MODE_CONTROL);
    outportb(CRTC_DATA, 0xA3);
}

void set_virtual_width(int width)
{
    outportb(CRTC_INDEX, CRTC_OFFSET);              // CRTC Offset 레지스터
    outportb(CRTC_DATA, (width / (4 * 2)) & 0xFF);  // 1바이트 = 4픽셀, 단위는 1/2로 지정
    _vwidth = width / 4;
}

void set_crtc_to_240_lines() {
    outportb(CRTC_INDEX, CRTC_VERT_DISP_ENABLE_END);       // Vertical Display End
    outportb(CRTC_DATA, 224);        // 줄 수를 400줄로 축소 (144 * 4 = 576 => 실제 400 비슷한 느낌으로 맞춤)
    _vheight = 240;
}

void set_bank_mask(int mask)
{
    outportb(SA_INDEX, SA_PLANE_MASK);
    outportb(SA_DATA, mask);
}

void select_bank(int bank)
{
    set_bank_mask(1 << bank);
}

void set_palette_multi(unsigned char* pal, int start, int count)
{
    outportb(DAC_INDEX, start);
    for (int i = 0; i < count; i++) {
        outportb(DAC_DATA, *pal++);
        outportb(DAC_DATA, *pal++);
        outportb(DAC_DATA, *pal++);
    }
}

// VRAM 상의 특정 위치에 bar를 그린다.
void draw_bar_to_vram( char *vdes, int offs, int width, int height, char color )
{
    int wloop = width / 4;
    for (int k = 0; k < 4; k++) {
        select_bank((offs + k) % 4);

        char* des = vdes;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < wloop; j ++) {
                *des++ = color;
            }
            des += _vwidth - wloop;
        }
    }
}

// VRAM 상의 특정 위치에 데이터를 전송한다
void send_data_to_vram( char *vdes, int offs, char* img, int size )
{
    for (int k = 0; k < 4; k++) {
        select_bank((offs + k) % 4);

        char* des = vdes;
        char *src = img + k;
        for (int i = 0; i < size; i++) {
            *des++ = *src;
            src += 4;
        }
    }
}

// VRAM 상의 특정 위치에 이미지를 전송한다
void send_image_to_vram( char *vdes, int offs, char* img, int width, int height )
{
    int wloop = width / 4;
    for (int k = 0; k < 4; k++) {
        select_bank((offs + k) % 4);

        char* des = vdes;
        char *src = img + k;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < wloop; j ++) {
                *des++ = *src;
                src += 4;
            }
            des += _vwidth - wloop;
        }
    }
}

void set_write_mode(int mode)
{
    outportb(GC_INDEX, GC_MODE);
    outportb(GC_DATA, inportb(GC_DATA) & 0xFC | mode);
}

// VRAM에서 VRAM으로 이미지를 복사한다
void copy_vram_to_vram( char *vdes, char *vsrc, int width, int height )
{
    set_write_mode(1);
    set_bank_mask(0xF);

    int wloop = width / 4;
    int skip = _vwidth - wloop;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < wloop; j++) {
            volatile register char dummy = *vsrc++;
            *vdes++ = 0x00; // 아무 값이나 쓴다.
        }
        vsrc += skip;
        vdes += skip;
    }
}

// VRAM에 이미지를 전송한다
void put_vram_to_vram( char *vram, int x, int y, char *vsrc, int width, int height )
{
    char *vdes = vram + y * _vwidth + x / 4;
    copy_vram_to_vram( vdes, vsrc, width, height );
}

void set_start_addr(unsigned int start_offs)
{
    outportb(CRTC_INDEX, CRTC_START_ADDR_HI);
    outportb(CRTC_DATA, (start_offs >> 8) & 0xff);
    outportb(CRTC_INDEX, CRTC_START_ADDR_LO);
    outportb(CRTC_DATA, start_offs & 0xff);
}

void wait_vsync()
{
    // while ((inportb(0x3DA) & 8) != 0);
    // while ((inportb(0x3DA) & 8) == 0);

    asm  mov    dx, 0x3DA
    wait_vsync_off:
    asm {
        in      al, dx
        test    al, 8
        jnz     wait_vsync_off  // VSYNC 끝날 때까지 기다림
    }
    wait_vsync_on:
    asm {
        in      al, dx
        test    al, 8
        jz      wait_vsync_on  // VSYNC 시작될 때까지 기다림
    }
}

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

unsigned char* make_image_mask( char* img, int width, int height, char trans_color, int endian )
{
    unsigned char* img_mask = (unsigned char*)malloc(width / 8 * height);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 8; j++) {
            unsigned char m = 0;
            for (int k = 0; k < 8; k++) {
                if (img[i * width + j * 8 + k] != trans_color) {
                    m |= (endian)? (1 << (7 - k)) : (1 << k);
                }
            }
            img_mask[i * (width / 8) + j] = m;
        }
    }
    return img_mask;
}

// VRAM to VRAM 스프라이트 전송
void send_sprite_vram_to_vram( char* des, char* src, int width, int height, unsigned char* mask )
{
    set_write_mode(1);

    int wloop = width / 4;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < wloop; j += 2) {
            unsigned char m = *mask++;

            int dummy = *(src + j);
            set_bank_mask(m);
            *(des + j) = 0x00;

            dummy = *(src + j + 1);
            set_bank_mask(m >> 4);
            *(des + j + 1) = 0x00;
        }
        src += _vwidth;
        des += _vwidth;
    }
}

void put_sprite_vram_to_vram( char *vram, int x, int y, char *src, int width, int height, unsigned char* mask )
{
    char* des = vram + y * _vwidth + x / 4;
    int idx = x & 0x3;
    width += (idx != 0) ? 4 : 0;
    send_sprite_vram_to_vram(des, src, width, height, mask);
}


#define MAX_TILE_WIDTH  (40)
#define MAX_TILE_HEIGHT (40)
#define MAX_SCREEN_WIDTH  (SCREEN_WIDTH * 2 + MAX_TILE_WIDTH)


#define TILE_WIDTH  (40)
#define TILE_HEIGHT (40)
#define MAP_WIDTH   (2048 / TILE_WIDTH)
#define MAP_HEIGHT  (SCREEN_HEIGHT / TILE_HEIGHT)

int _tile_width = TILE_WIDTH;
int _tile_height = TILE_HEIGHT;
int _tile_width_count = 2;
int _tile_full_tile_line = _vwidth*TILE_HEIGHT;
int _map_width = MAP_WIDTH;
int _map_height = MAP_HEIGHT;

unsigned short *_map_data = NULL;

int get_image_offs_from_map(int idx)
{
    return (idx / _tile_width_count) * _tile_full_tile_line + (idx % _tile_width_count) * _tile_width / 4;
}

void draw_bar(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color)
{
    int i, j;
    for (i = y1; i <= y2; i++) {
        for (j = x1; j <= x2; j++) {
            buf[i * vwidth + j] = color;
        }
    }
}

void draw_box(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color)
{
    int i, j;
    for (i = x1; i <= x2; i++) {
        buf[y1 * vwidth + i] = color;
        buf[y2 * vwidth + i] = color;
    }
    for (i = y1 + 1; i < y2; i++) {
        buf[i * vwidth + x1] = color;
        buf[i * vwidth + x2] = color;
    }
}

void draw_one_vert_line(int line_x, int map_x)
{
    for (int y = 0; y < _map_height; y++) {
        int tile = _map_data[y * _map_width + map_x];
        put_vram_to_vram( _vram, line_x, y * _tile_height, _asset_vram + get_image_offs_from_map(tile), _tile_width, _tile_height );
    }
}


void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();
    set_virtual_width(MAX_SCREEN_WIDTH);

    #ifdef LINES_240
    set_crtc_to_240_lines();
    #endif

    char *pal = load_file("son_til.pal");
    if (pal != NULL) {
        set_palette_multi((unsigned char *)pal, 0, 256);
        free(pal);
    }

    unsigned char *buf = load_file("son_til.img");
    if (buf != NULL) {
        int width = buf[0] | (buf[1] << 8);
        int height = buf[2] | (buf[3] << 8);
        send_image_to_vram(_asset_vram, 0, buf+4, width, height);
        free(buf);
        _tile_width = 8;
        _tile_height = 8;
        _tile_full_tile_line = _vwidth * _tile_height;
        _tile_width_count = width / _tile_width;
    }

    buf = load_file("son_map.map");
    if (buf != NULL) {
        _map_width = (unsigned)buf[0] | ((unsigned)buf[1] << 8);
        _map_height = (unsigned)buf[4] | ((unsigned)buf[5] << 8);
        _map_data = (unsigned short *)malloc(_map_width * _map_height * sizeof(short));
        memcpy(_map_data, buf+8, _map_width * _map_height * sizeof(short));
        free(buf);
    }

    buf = load_file("rap_00.img");
    unsigned char *msk;
    if (buf != NULL) {
        send_image_to_vram(_asset_vram + 300, 0, buf, 32, 32);
        msk = make_image_mask(buf, 32, 32, 0xFF, 0);
        free(buf);
    }

    int map_x = 0;
    for(int i = 0; i < SCREEN_WIDTH; i += _tile_height) {
        draw_one_vert_line( i, map_x );
        map_x++;
    }

    long idx = 0;

    // set_start_addr(idx);

    long bound_idx = idx + SCREEN_WIDTH;
    int speed = 1;

    int char_x = (SCREEN_WIDTH - 32) / 2;
    int char_y = (SCREEN_HEIGHT - 32) / 2;;
    int new_char_x = char_x;
    int new_char_y = char_y;

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
                    new_char_x = (char_x > 0) ? char_x - 4 : char_x - 0;
                    break;
                case 0x4D: // right
                    new_char_x = (char_x < SCREEN_WIDTH - 32) ? char_x + 4 : char_x +0;
                    break;
                case 0x48: // up
                    new_char_y = (char_y > 0) ? char_y - 4 : char_y - 0;
                    break;
                case 0x50: // down
                    new_char_y = (char_y < SCREEN_HEIGHT - 32) ? char_y + 4 : char_y + 0;
                    break;
                case '+':
                    if(speed < 8) speed++;
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

        int new_idx = idx + speed;
        if (new_idx + SCREEN_WIDTH > bound_idx) {
            if (map_x >= _map_width - SCREEN_WIDTH / _tile_width - 1) {
                status = 2;
            }
            if (bound_idx >= MAX_SCREEN_WIDTH) {
                bound_idx = 0 + SCREEN_WIDTH;
                new_idx = 0 + speed;
                rewind = 1;
            }
            draw_one_vert_line( bound_idx, map_x );
            map_x++;
            if (bound_idx - SCREEN_WIDTH - _tile_width >= 0) {
                copy_vram_to_vram( _vram + (bound_idx - _tile_width - SCREEN_WIDTH)/4, _vram + (bound_idx)/4, _tile_width, SCREEN_HEIGHT );
            }
            bound_idx += _tile_width;
        }

        wait_vsync();
        if (!rewind) {
            int blk_x1 = (idx + char_x) / _tile_width;
            int blk_y1 = (char_y) / _tile_height;
            int blk_x2 = (idx + char_x + 32 - 1) / _tile_width;
            int blk_y2 = (char_y + 32 - 1) / _tile_height;
            for(int by = blk_y1; by <= blk_y2; by++) {
                for(int bx = blk_x1; bx <= blk_x2; bx++) {
                    int tile = _map_data[(long)bx * _map_height + by];
                    put_vram_to_vram( _vram, bx * _tile_width, by * _tile_height, _asset_vram + get_image_offs_from_map(tile), _tile_width, _tile_height );
                }
            }
        } else {
            rewind = 0;
        }

        char_x = new_char_x;
        char_y = new_char_y;
        put_sprite_vram_to_vram(_vram + idx / 4, char_x, char_y, _asset_vram + 300, 32, 32, msk);

        idx = new_idx;
        set_start_addr(idx / 4);
    }

    if (msk) free(msk);
    if (_map_data) free(_map_data);

    set_screen_mode(0x03);
}
