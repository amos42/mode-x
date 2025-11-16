#ifndef __MODE_X_H_
#include "mode_x.h"
#endif
#ifndef __VGA_REG_H_
#include "vga_reg.h"
#endif
#include <malloc.h>


// 사각형 그리기
void draw_box(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color)
{
    int i;
    for (i = x1; i <= x2; i++) {
        buf[y1 * vwidth + i] = color;
        buf[y2 * vwidth + i] = color;
    }
    for (i = y1 + 1; i < y2; i++) {
        buf[i * vwidth + x1] = color;
        buf[i * vwidth + x2] = color;
    }
}


// 사각형 채우기
void draw_bar(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color)
{
    for (int i = y1; i <= y2; i++) {
        for (int j = x1; j <= x2; j++) {
            buf[i * vwidth + j] = color;
        }
    }
}


// 츠크 무늬 화면 생성
void make_check_screen(char* buf, int vwidth, int width, int height, char color1, char color2, char color3)
{
    for (int i = 0; i < height / 8; i++) {
        for (int j = 0; j < width / 8; j++) {
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


// 이미지 마스크 생성
unsigned char* make_image_mask(char* img, int offs, int width, int height, char trans_color, int endian)
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

// unsigned char* make_image_mask( char* img, int width, int height, char trans_color, int endian )
// {
//     unsigned char* img_mask = (unsigned char*)malloc(width / 8 * height);
//     for (int i = 0; i < height; i++) {
//         for (int j = 0; j < width / 8; j++) {
//             unsigned char m = 0;
//             for (int k = 0; k < 8; k++) {
//                 if (img[i * width + j * 8 + k] != trans_color) {
//                     m |= (endian)? (1 << (7 - k)) : (1 << k);
//                 }
//             }
//             img_mask[i * (width / 8) + j] = m;
//         }
//     }
//     return img_mask;
// }
