#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>
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

void set_virtual_width(int width)
{
    outportb(CRTC_INDEX, CRTC_OFFSET);              // CRTC Offset 레지스터
    outportb(CRTC_DATA, (width / (4 * 2)) & 0xFF);  // 1바이트 = 4픽셀, 단위는 1/2로 지정
    _vwidth = width / 4;
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

void set_start_addr(unsigned int start_offs)
{
    outportb(CRTC_INDEX, CRTC_START_ADDR_HI);
    outportb(CRTC_DATA, (start_offs >> 8) & 0xff);
    outportb(CRTC_INDEX, CRTC_START_ADDR_LO);
    outportb(CRTC_DATA, start_offs & 0xff);
}


void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();
    set_virtual_width(320 * 2);

    char* pal = (char*)malloc(256 * 3);
    FILE* fp = fopen("lych_00.pal", "rb");
    fread(pal, 256 * 3, 1, fp);
    fclose(fp);
    set_palette_multi((unsigned char *)pal, 0, 256);
    free(pal);

    char* buf = (char*)malloc(320 * 200);

    fp = fopen("lych_01.bin", "rb");
    fread(buf, 320 * 200, 1, fp);
    fclose(fp);
    send_image_to_vram(_vram, 0, buf, 320, 200);

    fp = fopen("lych_02.bin", "rb");
    fread(buf, 320 * 200, 1, fp);
    fclose(fp);
    send_image_to_vram(_vram + 320/4, 0, buf, 320, 200);
    
    free(buf);

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
