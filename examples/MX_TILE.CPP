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
    outportb(CRTC_DATA, 0xE3);
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
        send_image_to_vram(_vram + 0xB000, 0, buf+4, 40*2, 40);
        free(buf);
    }

    int idx0 = 0;
    for(int i = 0; i < screen_height; i += 40) {
        int idx = idx0;
        idx0 ^= 1;
        for(int j = 0; j < 320; j += 40) {
            put_vram_to_vram( _vram, j, i, _vram + 0xB000 + idx * 40/4 , 40, 40 );
            idx ^= 1;
        }
    }

    getch();

    set_screen_mode(0x03);
}
