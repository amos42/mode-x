#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <io.h>
#include "vga_reg.h"


#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   200

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

void set_crtc_to_240_lines() {
    outportb(CRTC_INDEX, CRTC_VERT_DISP_ENABLE_END);       // Vertical Display End
    outportb(CRTC_DATA, 224);        // 줄 수를 400줄로 축소 (144 * 4 = 576 => 실제 400 비슷한 느낌으로 맞춤)
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


#define TILE_W  40
#define TILE_H  40
#define MAP_W  (SCREEN_WIDTH/TILE_W)
#define MAP_H  (2096/TILE_H)

char map_data[MAP_W * MAP_H] = {
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,1,0,1,0,1,0,
};

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

void draw_one_line(int line_y, int map_y)
{
    char *vdes = _vram + line_y * TILE_W * _vwidth;

    for(int x = 0; x < 320/TILE_W; x++) {
        put_vram_to_vram( _vram, x * TILE_W, line_y, _asset_vram + map_data[(long)map_y * 8 + x] * TILE_W/4 , TILE_W, TILE_H );
    }
}

// #define LINES_240

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
        send_image_to_vram(_asset_vram, 0, buf+4, TILE_W*2, TILE_H);
        free(buf);
    }

    buf = load_file("rap_00.img");
    unsigned char *msk;
    if (buf != NULL) {
        send_image_to_vram(_asset_vram + TILE_W * _vwidth, 0, buf, 32, 32);
        msk = make_image_mask(buf, 32, 32, 0xFF, 0);
        free(buf);
    }

    int map_y = MAP_H - 1;
    for(int i = screen_height - TILE_H; i >= 0; i -= TILE_H) {
        draw_one_line( SCREEN_HEIGHT + i, map_y );
        map_y--;
    }

    int offs = 320/4*200;
    set_start_addr(320/4*200);

    int char_x = 160;
    int char_y = 100;
    int new_char_x = char_x;
    int new_char_y = char_y;

    long idx = 200;
    long limit_idx = 200;
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
                case 0x4B: // left
                    new_char_x = (char_x > 0) ? char_x - 4 : char_x - 0;
                    break;
                case 0x4D: // right
                    new_char_x = (char_x < 320 - 32) ? char_x + 4 : char_x +0;
                    break;
                case 0x48: // up
                    new_char_y = (char_y > 0) ? char_y - 4 : char_y - 0;
                    break;
                case 0x50: // down
                    new_char_y = (char_y < screen_height - 32) ? char_y + 4 : char_y + 0;
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

        int new_idx = idx - speed;
        if (new_idx < limit_idx) {
            if (map_y <= 0) {
                status = 2;
            }
            limit_idx -= 40;
            if (limit_idx < 0) {
                limit_idx = 240 - 40;
                new_idx = 240 - speed;
            }
            draw_one_line( limit_idx, map_y );
            map_y--;
            copy_vram_to_vram( _vram + (limit_idx + 240) * 320/4, _vram + limit_idx * 320/4, 320, 40 );
        }

        wait_vsync();
        
        long blk_x1 = char_x / 40;
        long blk_y1 = (idx + char_y) / 40;
        long blk_x2 = (char_x + 32 - 1) / 40;
        long blk_y2 = (idx + char_y + 32 - 1) / 40;
        for(int by = blk_y1; by <= blk_y2; by++) {
            for(int bx = blk_x1; bx <= blk_x2; bx++) {
                int tile = map_data[(long)by * (320/40) + bx];
                put_vram_to_vram( _vram, bx * 40, by * 40, _asset_vram + tile * 40/4 , 40, 40 );
            }
        }

        char_x = new_char_x;
        char_y = new_char_y;
        put_sprite_vram_to_vram(_vram + idx * 320/4, char_x, char_y, _asset_vram + 40 * _vwidth, 32, 32, msk);

        idx = new_idx;
        set_start_addr(idx * 320/4);
    }

    set_screen_mode(0x03);
}
