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

void set_virtual_width(int width)
{
    outportb(CRTC_INDEX, CRTC_OFFSET);              // CRTC Offset 레지스터
    outportb(CRTC_DATA, (width / (4 * 2)) & 0xFF);  // 1바이트 = 4픽셀, 단위는 1/2로 지정
    _vwidth = width / 4;
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


// VRAM 상의 특정 위치에 bar를 그린다.
void draw_bar_to_vram( char *vdes, int offs, int width, int height, char color )
{
    set_write_mode(0);

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


#define TILE_WIDTH  40
#define TILE_HEIGHT 40
#define MAX_SCREEN_WIDTH  (SCREEN_WIDTH * 2 + TILE_WIDTH)

void draw_one_line(int line_x, int start_idx)
{
    for(int i = 0; i < SCREEN_HEIGHT; i += TILE_HEIGHT) {
        put_vram_to_vram( _vram, line_x, i, _asset_vram + start_idx * TILE_WIDTH/4 , TILE_WIDTH, TILE_HEIGHT );
        start_idx ^= 1;
    }
}


void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();
    set_virtual_width(MAX_SCREEN_WIDTH);

    char *pal = load_file("tile.pal");
    if (pal != NULL) {
        set_palette_multi( (unsigned char *)pal, 0, 256 );
        free(pal);
    }
    char *buf = load_file("tile.img");
    if (buf != NULL) {
        send_image_to_vram( _asset_vram, 0, buf+4, TILE_WIDTH*2, TILE_HEIGHT );
        free(buf);
    }

    long idx = 0;

    int tile_idx = 0;
    for(int i = 0; i < SCREEN_WIDTH; i += TILE_WIDTH) {
        draw_one_line( idx + i, tile_idx );
        tile_idx ^= 1;
    }

    //set_start_addr(idx);

    long bound_idx = idx + SCREEN_WIDTH;
    int speed = 4;

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
            }
        }

        if (status != 0) {
            delay(100);
            continue;
        }

        idx += speed;
        if (idx + SCREEN_WIDTH > bound_idx) {
            if (bound_idx + TILE_WIDTH > MAX_SCREEN_WIDTH) {
                bound_idx = SCREEN_WIDTH;
                idx = 0 + speed;
            }
            draw_one_line( bound_idx, tile_idx );
            tile_idx ^= 1;
            if (bound_idx - SCREEN_WIDTH - TILE_WIDTH >= 0) {
                copy_vram_to_vram( _vram + (bound_idx - TILE_WIDTH - SCREEN_WIDTH)/4, _vram + (bound_idx)/4, TILE_WIDTH, SCREEN_HEIGHT );
                //draw_bar_to_vram( _vram + (bound_idx - TILE_WIDTH - SCREEN_WIDTH)/4, 0, TILE_WIDTH, SCREEN_HEIGHT, bound_idx - TILE_WIDTH - SCREEN_WIDTH );
            }
            bound_idx += TILE_WIDTH;
        }

        wait_vsync();

        set_start_addr(idx / 4);
    }

    set_screen_mode(0x03);
}
