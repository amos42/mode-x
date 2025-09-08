#include "mode_x.h"

// VRAM 상의 특정 위치에 이미지를 전송한다
void send_image_to_vram( char *vdes, int offs, char* img, int width, int height )
{
    for (int k = 0; k < 4; k++) {
        select_bank((offs + k) % 4);

        char* des = vdes;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width / 4; j++) {
                des[j] = img[i * width + j * 4 + k];
            }
            des += _vwidth;
        }
    }
}

// VRAM에 이미지를 전송한다
void put_image_to_vram( char *vram, int x, int y, char* img, int width, int height )
{
    char *vdes = vram + y * _vwidth + x / 4;
    send_image_to_vram( vdes, x % 4, img, width, height );
}
