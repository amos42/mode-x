// mode_x_bitmap_font.c
// 16비트 DOS용 C 코드 (Turbo C/Borland C/Watcom C 등) 예제
// VGA Mode X에서 쓰기 모드2를 사용해 8x8 비트맵 폰트를 출력
// 컴파일 후 DOSBox에서 실행 가능

#include <dos.h>
#include <conio.h>
//#include <stdint.h>

#define VGA_SEQ_INDEX   0x3C4
#define VGA_SEQ_DATA    0x3C5
#define VGA_GC_INDEX    0x3CE
#define VGA_GC_DATA     0x3CF
#define VGA_CRTC_INDEX  0x3D4
#define VGA_CRTC_DATA   0x3D5

#define VIDEO_SEG 0xA000

// 간단한 8x8 폰트 테이블 (ASCII 65 'A' 하나만 정의)
unsigned char font8x8[1][8] = {
    {0x18,0x24,0x24,0x42,0x7E,0x42,0x42,0x00}
};

// VGA 레지스터 설정
void set_mode13h() {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = 0x13;
    int86(0x10, &regs, &regs);
}

void set_mode3h() {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int86(0x10, &regs, &regs);
}

// Mode X 초기화 (chain-4 끄고 320x240)
void mode_x_init() {
    // Sequencer: disable chain-4
    outp(VGA_SEQ_INDEX, 0x04);
    outp(VGA_SEQ_DATA, 0x06);

    // Graphics Controller: set write mode 2 later
    outp(VGA_GC_INDEX, 0x05);
    outp(VGA_GC_DATA, 0x00);

    // CRTC: 늘려서 240 라인 (간단히 일부만 조정)
    // outp(VGA_CRTC_INDEX, 0x09);
    // outp(VGA_CRTC_DATA, 0x40);
}

// 쓰기 모드2 설정
void set_write_mode2() {
    outp(VGA_GC_INDEX, 0x05);
    outp(VGA_GC_DATA, 0x02); // write mode 2
}

// 비트마스크 레지스터 설정
void set_bitmask(unsigned char mask) {
    outp(VGA_GC_INDEX, 0x08);
    outp(VGA_GC_DATA, mask);
}

// 글자 하나 출력 (8x8)
void draw_char_mode2(int x, int y, char ch, char color) {
    unsigned short offset = (y * 80) + (x >> 2); // 바이트 오프셋
    unsigned char* vram = (unsigned char *)MK_FP(VIDEO_SEG, 0);

    set_write_mode2();

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

int main() {
    set_mode13h();
    mode_x_init();

    draw_char_mode2(40, 40, 'A', 12);
    draw_char_mode2(60, 40, 'A', 14);

    getch();
    set_mode3h();
    return 0;
}
