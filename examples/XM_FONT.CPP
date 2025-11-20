// mode_x_bitmap_font.c
// 16비트 DOS용 C 코드 (Turbo C/Borland C/Watcom C 등) 예제
// VGA Mode X에서 쓰기 모드2를 사용해 8x8 비트맵 폰트를 출력
// 컴파일 후 DOSBox에서 실행 가능

#include <dos.h>
#include <conio.h>
//#include <stdint.h>
#include "../src/MODE_X.H"
#include "../src/VGA_REG.H"

#include "../src/VGA_CTL.CPP"
#include "../src/GDI.CPP"


// 간단한 8x8 폰트 테이블 (ASCII 65 'A' 하나만 정의)
unsigned char font8x8[1][8] = {
    {0x18,0x24,0x24,0x42,0x7E,0x42,0x42,0x00}
};

// 글자 하나 출력 (8x8)
void draw_char_mode2(int x, int y, char ch, char color) {
    unsigned char *vram = (unsigned char *)_vram;
    unsigned short offset = (y * 80) + (x >> 2); // 바이트 오프셋

    set_write_mode(2);

    for (int row = 0; row < 8; row++) {
        unsigned char fontrow = font8x8[0][row];

        // 비트마스크 = fontrow
        set_bitmask(fontrow);

        // 래치 채우기 (dummy read)
        volatile char dummy = vram[offset];

        // 실제 쓰기: 색상 한 바이트
        vram[offset] = color;

        offset += 80; // 다음 라인
    }
}

void main()
{
    set_screen_mode(0x13);
    turn_off_chain4();

    // Graphics Controller: set write mode 2 later
    outp(GC_INDEX, GC_MODE);
    outp(GC_DATA, 0x00);

    draw_char_mode2(40, 40, 'A', 12);
    draw_char_mode2(60, 40, 'A', 14);

    getch();
    set_screen_mode(0x03);
}
