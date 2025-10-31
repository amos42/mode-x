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
    outportb(SA_DATA, inportb(SA_DATA) & ~8);
    // outportb(SA_DATA, 0x06);

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

void set_write_mode(int mode)
{
    outportb(GC_INDEX, GC_MODE);
    outportb(GC_DATA, inportb(GC_DATA) & 0xFC | mode);
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

// VRAM 상의 특정 위치에 데이터를 전송한다
void send_data_to_vram( char *vdes, int offs, char* img, int size )
{
    for (int k = 0; k < 4; k++) {
        select_bank((offs + k) % 4);

        char *des = vdes;
        char *src = img + k;
        for (int i = 0; i < size/4; i++) {
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

        char *des = vdes;
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


// #define MAX_TILE_WIDTH  (40)
// #define MAX_TILE_HEIGHT (40)
#define MAX_TILE_WIDTH  (8)
#define MAX_TILE_HEIGHT (8)
#define MAX_SCROLL_TICK_SIZE (16)
// #define MAX_SCREEN_WIDTH  (SCREEN_WIDTH * 2 + MAX_TILE_WIDTH)


#define TILE_WIDTH  (40)
#define TILE_HEIGHT (40)
#define MAP_WIDTH   (2048 / TILE_WIDTH)
#define MAP_HEIGHT  (SCREEN_HEIGHT / TILE_HEIGHT)

int _tile_width = TILE_WIDTH;
int _tile_height = TILE_HEIGHT;
int _tile_count = 0;
// int _tile_width_count = 2;
// int _tile_full_tile_line = _vwidth*TILE_HEIGHT;
int _map_width = MAP_WIDTH;
int _map_height = MAP_HEIGHT;

unsigned short *_map_data = NULL;

unsigned int get_image_offs_from_map(unsigned int idx)
{
    // int xidx = idx % _tile_width_count;
    // int yidx = idx / _tile_width_count;
    // return yidx * _tile_full_tile_line + xidx * _tile_width / 4;
    return idx * _tile_width / 4 * _tile_height;
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

// VRAM에서 VRAM으로 이미지를 복사한다
void copy_vram_to_vram2( char *vdes, char *vsrc, int width, int height )
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
        //vsrc += skip;
        vdes += skip;
    }
}

void put_vram_to_vram2( char *vram, int x, int y, char *vsrc, int width, int height )
{
    char *vdes = vram + y * _vwidth + x / 4;
    copy_vram_to_vram2( vdes, vsrc, width, height );
}


void draw_one_vert_line(char *vram, int line_x, int map_x)
{
    int idx = map_x;
    for (int y = 0; y < _map_height; y++) {
        unsigned int tile = _map_data[y*_map_width + map_x];
        put_vram_to_vram2( vram, line_x, y * _tile_height, _asset_vram + get_image_offs_from_map(tile), _tile_width, _tile_height );
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

void recover_bg_area_with_bg_mask(int start_map_x, int start_pos, unsigned long* bg_mask)
{
    // static int cl = 1;        
    for(int i = 0; i < SCREEN_WIDTH/_tile_width; i ++) {
        unsigned long mask = bg_mask[i];
        if (mask == 0x0) continue;

        long map_idx = start_map_x + i;
        for(int j = 0; j < SCREEN_HEIGHT/_tile_height; j ++) {
            if (mask & 1) {
                unsigned int tile = _map_data[j*_map_width + map_idx];
                put_vram_to_vram2( _vram, start_pos + i * _tile_width, j * _tile_height, _asset_vram + get_image_offs_from_map(tile), _tile_width, _tile_height );
                // draw_bar_to_vram( _vram + scroll_idx / 4 + i * _tile_width / 4 + j * _tile_height * _vwidth, 0, _tile_width, _tile_height, cl );
            }
            mask >>= 1;
        }
        bg_mask[i] = 0x0;
    }
    // cl = cl++ % 32;
}


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

void load_tile(char *tile_filename, char *vram)
{
    unsigned char *buf = load_file(tile_filename);
    if (buf == NULL) return;
    _tile_width = (unsigned)buf[0] | ((unsigned)buf[1] << 8);
    _tile_height = (unsigned)buf[4] | ((unsigned)buf[5] << 8);
    _tile_count = (unsigned)buf[8] | ((unsigned)buf[9] << 8);
    send_data_to_vram(vram, 0, buf + 16, _tile_width * _tile_height * _tile_count);
    free(buf);
}

void load_tile_from_image(char *tile_img_filename, int tile_width, int tile_height, char *vram)
{
    unsigned char *buf = load_file(tile_img_filename);
    if (buf == NULL) return;

    int width = (unsigned)buf[0] | ((unsigned)buf[1] << 8);
    int height = (unsigned)buf[2] | ((unsigned)buf[3] << 8);

    unsigned char *buf2 = buf + 4;
    unsigned char *vram2 = (unsigned char *)vram;
    for(int i = 0; i < height; i += tile_height) {
        for(int j = 0; j < width; j += tile_width) {
            for (int k = 0; k < tile_height; k++) {
                send_data_to_vram(vram2, 0, buf2 + k * width, tile_width);
                vram2 += tile_width / 4;
            }
            buf2 += tile_width;
        }
        buf2 += width * (tile_height - 1);
    }
    free(buf);

    _tile_width = tile_width;
    _tile_height = tile_height;
    _tile_count = (width / _tile_width) * (height / _tile_height);
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

    // load_tile("son_tile.til", _asset_vram);
    load_tile_from_image("son_til.img", 8, 8, _asset_vram);
    load_map("son_map.map");

    set_virtual_width(SCREEN_WIDTH * 2 + _tile_width);

    char *buf = load_file("sonic.img");
    unsigned char *msk;
    if (buf != NULL) {
        send_image_to_vram(_asset_vram + 20000/4, 0, buf+4, 48, 24);
        msk = make_image_mask(buf+4, 48, 24, 0xFF, 0);
        free(buf);
    }

    int start_pos = 0;
    int next_pos = start_pos + _tile_width;
    int start_map_x = 0;    
    
    for(int i = 0; i < SCREEN_WIDTH/_tile_width + 1; i++) {
        draw_one_vert_line( _vram, start_pos + i * _tile_width, start_map_x + i );
    }
    long scroll_pos = 0;

    int speed = 4;

    int char_x = 80;
    int char_y = 120;
    int new_char_x = char_x;
    int new_char_y = char_y;

    unsigned long bg_mask[320/8] = {0, };

    int status = 0;
    int new_block = 0;
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
                    if(speed < MAX_SCROLL_TICK_SIZE) speed+=4;
                    break;
                case '-':
                    if(speed > 4) speed-=4;
                    break;
            }
        } 

        if (status != 0) {
            delay(100);
            continue;
        }

        scroll_pos += speed;

        // wait_vsync();

        if (scroll_pos >= next_pos) {
            if (start_map_x + SCREEN_WIDTH / _tile_width >= _map_width - 1) {
                status = 2;
                continue;
            }

            if (next_pos + SCREEN_WIDTH >= (SCREEN_WIDTH * 2 + _tile_width)) {
                start_pos = 0;
                scroll_pos = 0;
                rewind = 1;
            } else {
                start_pos += _tile_width;
            }

            new_block = 1;
        }

        if (new_block) {
            // start_pos += _tile_width;
            next_pos = start_pos + _tile_width;

            start_map_x++;
            draw_one_vert_line( _vram, start_pos + SCREEN_WIDTH, start_map_x + (SCREEN_WIDTH / _tile_width) );
            // if (start_pos >= _tile_width) {
            //    copy_vram_to_vram( _vram + (start_pos - _tile_width)/4, _vram + (start_pos + SCREEN_WIDTH)/4, _tile_width, SCREEN_HEIGHT );
            // }
            // new_block = 0;
        }

        set_start_addr(scroll_pos / 4);
        wait_vsync();

        if (new_block) {
            // start_pos += _tile_width;
            // next_pos = start_pos + _tile_width;

            // start_map_x++;
            // draw_one_vert_line( _vram, start_pos + SCREEN_WIDTH, start_map_x + (SCREEN_WIDTH / _tile_width) );
            if (start_pos >= _tile_width) {
               copy_vram_to_vram( _vram + (start_pos - _tile_width)/4, _vram + (start_pos + SCREEN_WIDTH)/4, _tile_width, SCREEN_HEIGHT );
            }
            new_block = 0;
        }

        if (!rewind) {
            recover_bg_area_with_bg_mask(start_map_x, start_pos, bg_mask);
        } else {
            clear_bg_mask(bg_mask);
            rewind = 0;
        }

        char_x = new_char_x;
        char_y = new_char_y;
        put_sprite_vram_to_vram(_vram + scroll_pos / 4, char_x, char_y, _asset_vram + 20000/4, 48, 24, msk);
        do_bg_masking(char_x, char_y, char_x+48-1, char_y+24-1, bg_mask);
    }

    if (msk) free(msk);
    if (_map_data) free(_map_data);

    set_screen_mode(0x03);
}
