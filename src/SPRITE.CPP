#include "mode_x.h"
#include <stdlib.h>

// 이미지 마스크 생성
unsigned char* make_image_mask(char* img, int img_x, int img_y, char trans_color)
{
    unsigned char* img_mask = (unsigned char*)malloc(img_x / 8 * img_y);

    for (int i = 0; i < img_y; i++) {
        for (int j = 0; j < img_x / 8; j++) {
            unsigned char m = 0;
            for (int k = 0; k < 8; k++) {
                if (img[i * img_x + j * 8 + k] != trans_color) {
                    m |= (1 << (7 - k));
                    //m |= (1 << k);
                }
            }
            img_mask[i * (img_x / 8) + j] = m;
        }
    }

    return img_mask;
}

// 이미지 크기를 4의 배수로 맞추기
char* make_shift_image(char* img, int width, int height, int* new_width, int* new_height)
{
    int w = width + 3;
    w &= ~0x3;
    int h = height + 3;
    h &= ~0x3;

    char* new_img = (char*)malloc(w * h);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (i < height && j < width) {
                new_img[i * w + j] = img[i * width + j];
            }
            else {
                new_img[i * w + j] = 0;
            }
        }
    }

    *new_width = w;
    *new_height = h;

    return new_img;
}

// VRAM to VRAM 스프라이트 전송
void put_sprite_vram_to_vram(char* des, char* src, int width, int height, unsigned char* mask)
{
    set_write_mode(1);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 4; j += 2) {
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

    set_write_mode(0);
}

// 스프라이트 그리기
void put_sprite_vram_to_vram2(char *vram, int x, int y, char* imgs[4], int width, int height, unsigned char* mask[4])
{
    char* des = vram + y * _vwidth + x / 4;
    int idx = x & 0x3;
    put_sprite_vram_to_vram(des, imgs[idx], width, height, mask[idx]);
}
