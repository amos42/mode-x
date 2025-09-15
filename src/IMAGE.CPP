#include "mode_x.h"

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
void copy_vram_to_vram( char *vdes, char *vsrc, int width, int height )
{
    set_write_mode(1);
    set_bank_mask(0xF);

    int wloop = width / 4;
    int skip = _vwidth - wloop;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < wloop; j++) {
            int dummy = *vsrc++;
            *vdes++ = 0x00;
        }
        vsrc += skip;
        vdes += skip;
    }
}

// VRAM에 이미지를 전송한다
void put_vram_to_vram( char *vram, char *vsrc, int x, int y, char* img, int width, int height )
{
    char *vdes = vram + y * _vwidth + x / 4;
    copy_vram_to_vram( vdes, vsrc, width, height );
}
