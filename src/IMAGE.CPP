#ifndef __MODE_X_H_
#include "mode_x.h"
#endif
#ifndef __VGA_REG_H_
#include "vga_reg.h"
#endif



// VRAM 상의 특정 위치에 이미지를 전송한다
void send_image_to_vram( char *vdes, int offs, char* img, int width, int height )
{
    set_write_mode(0);

    int wloop = width / 4;
    int skip = _vwidth - wloop;
    for (int k = 0; k < 4; k++) {
        select_bank((offs + k) % 4);

        char* des = vdes;
        char *src = img + k;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < wloop; j ++) {
                *des++ = *src;
                src += 4;
            }
            des += skip;
        }
    }
}

// VRAM에 이미지를 전송한다
void put_image_to_vram( char *vram, int x, int y, char* img, int width, int height )
{
    char *vdes = vram + y * _vwidth + x / 4;
    send_image_to_vram( vdes, x % 4, img, width, height );
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
void put_vram_to_vram( char *vram, char *vsrc, int x, int y, int width, int height )
{
    char *vdes = vram + y * _vwidth + x / 4;
    copy_vram_image_to_vram_image( vdes, vsrc, width, height );
}

void copy_vram_to_vram_image( char *vdes, char *vsrc, int width, int height )
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
        vdes += skip;
    }
}

void put_vram_to_vram_image( char *vram, int x, int y, char *vsrc, int width, int height )
{
    char *vdes = vram + y * _vwidth + x / 4;
    copy_vram_to_vram_image( vdes, vsrc, width, height );
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
