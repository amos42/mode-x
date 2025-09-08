#include "mode_x.h"


// void put_font_to_vram(int xpage, int ypage, unsigned char* font, int start_x, int start_y, int width, int height, char color)
// {
//     char* des = get_vram_addr(xpage, ypage, start_x, start_y);

//     set_write_mode(2);

//     set_plane_mask(0xF);
//     for (int i = 0; i < height; i++) {
//         for (int j = 0; j < width / 8; j ++) {
//             unsigned char m = *font++;
//             set_bitmask(m);

//             volatile char dummy = des[j];
//             des[j] = color;
//         }
//         des += 320 / 4 * PAGE_HORIZ_COUNT;
//     }

//     set_write_mode(0);
// }
