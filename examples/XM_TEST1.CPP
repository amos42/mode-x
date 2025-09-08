// mode_x_bitmap_font.c
// 16비트 DOS용 C 코드 (Turbo C/Borland C/Watcom C 등) 예제
// VGA Mode X에서 쓰기 모드2를 사용해 8x8 비트맵 폰트를 출력
// 컴파일 후 DOSBox에서 실행 가능

#include <dos.h>
#include <conio.h>
//#include <stdint.h>
#include <stdio.h>
#include <malloc.h>


#define VGA_SEQ_INDEX   0x3C4
#define VGA_SEQ_DATA    0x3C5
#define VGA_GC_INDEX    0x3CE
#define VGA_GC_DATA     0x3CF
#define VGA_CRTC_INDEX  0x3D4
#define VGA_CRTC_DATA   0x3D5

#define VIDEO_SEG 0xA000


// VGA 레지스터 설정
void set_mode13h()
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = 0x13;
    int86(0x10, &regs, &regs);
}

void set_mode3h()
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int86(0x10, &regs, &regs);
}

// Mode X 초기화 (chain-4 끄고 320x240)
void set_320x240()
{
    outp(VGA_CRTC_INDEX, 0x12); // Vertical Display End
    outp(VGA_CRTC_DATA, 0xE0);

    // CRTC: 늘려서 240 라인 (간단히 일부만 조정)
    // outp(VGA_CRTC_INDEX, 0x09); // Maximum Scan Line
    // outp(VGA_CRTC_DATA, 0x40);
}

// Mode X 초기화 (chain-4 끄고 320x240)
void mode_x_init()
{
    // Sequencer: disable chain-4
    outp(VGA_SEQ_INDEX, 0x04);
    outp(VGA_SEQ_DATA, 0x06);

    // // Graphics Controller: set write mode 2 later
    // outp(VGA_GC_INDEX, 0x05);
    // outp(VGA_GC_DATA, 0x00);

    // CRTC: 늘려서 240 라인 (간단히 일부만 조정)
    // outp(VGA_CRTC_INDEX, 0x09);
    // outp(VGA_CRTC_DATA, 0x40);
}

int main()
{
    set_mode13h();
    set_320x240();
    // mode_x_init();

    char *vram = (char *)MK_FP(VIDEO_SEG, 0);
    for (long i = 0; i < 65536L; i++)
    {
        vram[i] = 0x01;
    }
    
    getch();

    set_mode3h();

    return 0;
}
