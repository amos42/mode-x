#include "mode_x.h"

// 화면을 특정 색으로 지운다
void clear_screen( char *vram, char color )
{
    set_bank_mask(0xF);

    for( int i = 0; i < _vheight; i++ ) {
        char *vdes = vram;
        for( int j = 0; j < _vwidth; j++ ) {
            *vdes++ = color;
        }
        vram += _vwidth;
    }
}
