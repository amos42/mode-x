#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <io.h>
#include "../src/VGA_REG.H"


// #define LINES_240

#define SCREEN_WIDTH    320
#ifdef LINES_240
#define SCREEN_HEIGHT   240
#else
#define SCREEN_HEIGHT   200
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
    outportb(CRTC_DATA, 0xE3);
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
void copy_vram_image_to_vram_image( char *vdes, char *vsrc, int width, int height )
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
    copy_vram_image_to_vram_image( vdes, vsrc, width, height );
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


#define TILE_WIDTH  40
#define TILE_HEIGHT 40
#define MAX_SCREEN_HEIGHT  (SCREEN_HEIGHT * 2 + TILE_HEIGHT)

void draw_one_line(int line_y, int start_idx)
{
    for(int i = 0; i < SCREEN_WIDTH; i += TILE_WIDTH) {
        put_vram_to_vram( _vram, i, line_y, _asset_vram + start_idx * TILE_WIDTH/4 , TILE_WIDTH, TILE_HEIGHT );
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
