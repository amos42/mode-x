#ifndef __MODE_X_H_
#include "mode_x.h"
#endif
#ifndef __VGA_REG_H_
#include "vga_reg.h"
#endif
#include <dos.h>


char* _vram = (char*)0xA0000000;
int _vwidth = 320 / 4;
int _vheight = 200;


// 화면 모드를 설정한다
void set_screen_mode(int mode)
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}

// 세로 해상도를 240으로 변경한다
void set_crtc_to_240_lines()
{
    outportb(CRTC_INDEX, 0x12); // Vertical Display End
    outportb(CRTC_DATA, 0xE0);  // 줄 수를 240줄로 확대

    outportb(CRTC_INDEX, 0x09); // Maximum Scan Line
    outportb(CRTC_DATA, 0x40);

    _vheight = 240;
}

// chain-4 로직을 끈다
void turn_off_chain4()
{
    // none-chain-4 모드
    outportb(SA_INDEX, SA_MEMORY_MODE);
    outportb(SA_DATA, inportb(SA_DATA) & ~8);
    // outportb(SA_DATA, 0x06);

    // 스캔 주기 60Hz
    outportb(MISC_OUTPUT, 0xE3);

    // 1byte 전송 모드
    outportb(CRTC_INDEX, CRTC_UNDERLINE_LOCATTION);
    outportb(CRTC_DATA, 0x00);
    outportb(CRTC_INDEX, CRTC_MODE_CONTROL);
    outportb(CRTC_DATA, 0xE3);

    // CRTC 수직 해상도 관련 레지스터의 조작을 막는다.
    // outportb(CRTC_INDEX, CRTC_VERTICAL_SYNC_END);
    // outportb(CRTC_DATA, inportb(CRTC_DATA) & 0x7F);
}

// 뱅크 마스크 설정
void set_bank_mask(int mask)
{
    outportb(SA_INDEX, SA_PLANE_MASK);
    outportb(SA_DATA, mask);
}

// 뱅크 하나를 선택한다
void select_bank(int bank)
{
    set_bank_mask(1 << bank);
}

// 쓰기 모드를 설정한다
void set_write_mode(int mode)
{
    outportb(GC_INDEX, GC_MODE);
    outportb(GC_DATA, inportb(GC_DATA) & 0xFC | mode);
}

// 쓰기 함수를 설정한다
void set_write_func(int op_mode)
{
    outportb(GC_INDEX, GC_FUNC);
    outportb(GC_DATA, inportb(GC_FUNC) & 0xE7 | (op_mode << 3));
}

// 수직 동기 신호를 기다린다
void wait_vsync()
{
    // while ((inportb(0x3DA) & 8) != 0);
    // while ((inportb(0x3DA) & 8) == 0);

    asm  {
        mov    dx, 0x3DA
    }
    wait_vsync_off:
    asm {
        in      al, dx
        test    al, 8
        jnz     wait_vsync_off  // VSYNC 끝날 때까지 기다림
    }
    wait_vsync_on:
    asm {
        in      al, dx
        test    al, 8
        jz      wait_vsync_on  // VSYNC 시작될 때까지 기다림
    }
}

// 시작 오프셋을 설정한다
void set_start_addr(unsigned int start_offs)
{
    outportb(CRTC_INDEX, CRTC_START_ADDR_HI);
    outportb(CRTC_DATA, (start_offs >> 8) & 0xff);
    outportb(CRTC_INDEX, CRTC_START_ADDR_LO);
    outportb(CRTC_DATA, start_offs & 0xff);
}

// 시작 오프셋의 상위 주소만 설정한다
void set_start_addr_hi(unsigned char start_offs_hi)
{
    outportb(CRTC_INDEX, CRTC_START_ADDR_HI);
    outportb(CRTC_DATA, start_offs_hi);
}

// 가상 폭을 설정한다
void set_virtual_width(int width)
{
    outportb(CRTC_INDEX, CRTC_OFFSET);              // CRTC Offset 레지스터
    outportb(CRTC_DATA, (width / (4 * 2)) & 0xFF);  // 1바이트 = 4픽셀, 단위는 1/2로 지정

    _vwidth = width / 4;
}

// 비트마스크 레지스터 설정
void set_bitmask(unsigned char mask)
{
    outportb(GC_INDEX, GC_BITMASK);
    outportb(GC_DATA, mask);
}
