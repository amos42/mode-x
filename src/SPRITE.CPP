#include "mode_x.h"
#include <stdlib.h>

// unsigned char* make_image_mask(char* img, int img_x, int img_y, char trans_color)
// {
//     unsigned char* img_mask = (unsigned char*)malloc(img_x / 8 * img_y);
//     for (int i = 0; i < img_y; i++) {
//         for (int j = 0; j < img_x / 8; j++) {
//             unsigned char m = 0;
//             for (int k = 0; k < 8; k++) {
//                 if (img[i * img_x + j * 8 + k] != trans_color) {
//                     m |= (1 << (7 - k));
//                     //m |= (1 << k);
//                 }
//             }
//             img_mask[i * (img_x / 8) + j] = m;
//         }
//     }
//     return img_mask;
// }

// void put_vram_to_vram_sprite(int xpage, int ypage, int s_xpage, int s_ypage, int start_x, int start_y, int s_start_x, int s_start_y, int width, int height, unsigned char* mask)
// {
//     char* src = get_vram_addr(s_xpage, s_ypage, s_start_x, s_start_y);
//     char* des = get_vram_addr(xpage, ypage, start_x, start_y);

//     set_write_mode(1);

//     start_x &= ~0x3;
//     for (int i = 0; i < height; i++) {
//         for (int j = 0; j < width / 4; j += 2) {
//             unsigned char m = *mask++;

//             int dummy = *(src + j);
//             set_bank_mask(m);
//             *(des + j) = 0x00;

//             dummy = *(src + j + 1);
//             set_plane_mask(m >> 4);
//             *(des + j + 1) = 0x00;
//         }
//         src += 320 / 4 * PAGE_HORIZ_COUNT;
//         des += 320 / 4 * PAGE_HORIZ_COUNT;
//     }

//     set_write_mode(0);
// }

// char *make_shift_image( char *img, int width, int height, int *new_width, int *new_height )
// {
//     int w = width + 3;
//     w &= ~0x3;
//     int h = height + 3;
//     h &= ~0x3;

//     char *new_img = (char*)malloc( w * h );
//     for( int i = 0; i < h; i++ ) {
//         for( int j = 0; j < w; j++ ) {
//             if( i < height && j < width ) {
//                 new_img[i*w + j] = img[i*width + j];
//             }
//             else {
//                 new_img[i*w + j] = 0;
//             }
//         }
//     }

//     *new_width = w;
//     *new_height = h;

//     return new_img;
// }

// void put_sprite2( int x, int y, char *img, int width, int height, unsigned char *mask )
// {
//     int idx = x & 0x3;
//     int offset = idx * widrh;
//     put_vram_to_vram_sprite( x, y, vram + offset, width, height, mask );
// }
