#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <io.h>
#include "vga_reg.h"


char* _vram = (char*)0xA0000000;
int _vwidth = 320 / 4;
int _vheight = 200;


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

void set_write_mode(int mode)
{
    outportb(GC_INDEX, GC_MODE);
    outportb(GC_DATA, inportb(GC_DATA) & 0xFC | mode);
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

void put_image_to_vram( char *_vram, int x, int y, char *img, int width, int height )\
{
    char* des = _vram + y * _vwidth + x / 4;
    send_image_to_vram( des, x % 4, img, width, height );
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

// 스프라이트 그리기
// void put_sprite_vram_to_vram( char *vram, int x, int y, char* imgs[4], int width, int height, unsigned char* mask[4] )
// {
//     char* des = vram + y * _vwidth + x / 4;
//     int idx = x & 0x3;
//     width += (idx != 0) ? 4 : 0;
//     copy_sprite_vram_to_vram(des, imgs[idx], width, height, mask[idx]);
// }
void put_sprite_vram_to_vram( char *vram, int x, int y, char *src, int width, int height, unsigned char* mask )
{
    char* des = vram + y * _vwidth + x / 4;
    int idx = x & 0x3;
    width += (idx != 0) ? 4 : 0;
    send_sprite_vram_to_vram(des, src, width, height, mask);
}

void put_image_to_vram_mask( char *vram, int x, int y, unsigned char* mask, int width, int height)
{
    char* des = vram + y * _vwidth + x / 4;

    set_write_mode(2);

    set_bank_mask(0xF);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 4; j += 2) {
            unsigned char m = *mask++;
            des[j] = m;
            des[j + 1] = m >> 4;
        }
        des += _vwidth;
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

void draw_bar(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color)
{
    int i, j;
    for (i = y1; i <= y2; i++) {
        for (j = x1; j <= x2; j++) {
            buf[i * vwidth + j] = color;
        }
    }
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
