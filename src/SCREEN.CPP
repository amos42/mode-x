#include "mode_x.h"

// 화면을 특정 색으로 지운다
void clear_screen( char *vram, char color )
{
    set_bank_mask(0xF);

    int skip = _vwidth - 320;
    for( int i = 0; i < _vheight; i++ ) {
        for( int j = 0; j < _vwidth; j++ ) {
            *vram++ = color;
        }
        vram += skip;
    }
}
