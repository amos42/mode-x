#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include "vga_reg.h"


char *vram = (char *)0xA0000000; // VGA 메모리 시작 주소

void set_plane_mode()
{
  outportb(SA_INDEX, SA_MEMORY_MODE);
  outportb(SA_DATA, inportb(SA_DATA) & ~8);

  outportb(CRTC_INDEX, CRTC_UNDERLINE_LOCATTION);
  outportb(CRTC_DATA, 0x00);
  outportb(CRTC_INDEX, CRTC_MODE_CONTROL);
  outportb(CRTC_DATA, 0xa3);
}

void set_plane_mask(int plane_mask)
{
    outportb(SA_INDEX, SA_PLANE_MASK);
    outportb(SA_DATA, plane_mask);
}

void select_plane(int plane)
{
    set_plane_mask(1 << plane);
}

void set_start_index(unsigned int idx)
{
  outportb(CRTC_INDEX, CRTC_START_ADDR_HI);
  outportb(CRTC_DATA, (idx >> 8) & 0xff);
  outportb(CRTC_INDEX, CRTC_START_ADDR_LO);
  outportb(CRTC_DATA, idx & 0xff);
}

void set_write_mode(int mode)
{
    outportb(GC_INDEX, GC_MODE);
    outportb(GC_DATA, inportb(GC_DATA) & 0xFC | mode);
}

void set_write_func(int op_mode)
{
    outportb(GC_INDEX, GC_FUNC);
    outportb(GC_DATA, inportb(GC_FUNC) & 0xE7 | (op_mode << 3));
}

void set_enable_set_reset_mask(int plane_mask)
{
    outportb(GC_INDEX, GC_ENABLE_SET_RESET);
    outportb(GC_DATA, plane_mask);
}

void set_one_pixel_mode()
{
    set_plane_mask(0xF);
    set_write_mode(3);
    set_enable_set_reset_mask(0xF);
}

void set_pallete( int color, char red, char green, char blue )
{
    outportb(DAC_INDEX, color & 0xFF);
    outportb(DAC_DATA, red);
    outportb(DAC_DATA, green);
    outportb(DAC_DATA, blue);
}

void set_pallete_multi( int start_color, int count,
                        char *red, char *green, char *blue )
{
    outportb(DAC_INDEX, start_color & 0xFF);
    for( int i = 0; i < count; i++ ){
        outportb(DAC_DATA, *red++);
        outportb(DAC_DATA, *green++);
        outportb(DAC_DATA, *blue++);
    }
}

void set_color(unsigned char color)
{
    outportb(GC_INDEX, GC_SET_RESET); // Set/Reset
    outportb(GC_DATA, color & 0x0F);  // Set color (0-15)
}

void put_pixel(int x, int y)
{
    unsigned short offset = y * (640/8) + (x >> 3);
    unsigned unsigned char mask = (unsigned char)0x80 >> (x & 0x07);

    char dummy = vram[offset]; // latch를 채우기 위한 더미 읽기 동작
                               // 최적화 과정에서 삭제되는 경우도 있으니 주의!
    vram[offset] = mask;  // 모드 3에서는 mask를 쓴다.
}

void clear_screen0(unsigned char color)
{
    int p;
    unsigned i;
    char bits;

    for( p = 0; p < 4; p++ ){
        set_plane_mask(1 << p);
        bits = ((color >> p) & 0x01) ? 0xFF : 0x00;
        for(i = 0; i < 640U/8*480; i++) {
            vram[i] = bits;
        }
    }
}

void clear_screen(unsigned char color)
{
    unsigned i;

    set_color(color);

    for(i = 0; i < 640U/8*480; i++) {
        vram[i] = 0xFF;  // 모드 3에서는 mask를 쓴다.
    }
}

void main()
{
    int i, j;

    asm {
        mov ax, 0x12
        int 0x10  // 화면을 그래픽 모드로 전환
    }

    clear_screen0(0x1);

    set_one_pixel_mode();

    //clear_screen(0x1);

    for(i = 0; i <= 0x0f; i++){
	//for(j = 0; j < 10; j++) {
        set_color(i);
	    put_pixel(10 + i, 10 + i);
	    put_pixel(10 + 0xF - i, 10 + i);
	//}
    }
    getch();
    for(i = 0; i <= 0x0f; i++){
	//for(j = 0; j < 10; j++) {
        set_color(15-i);
        put_pixel(10 + i, 10 + i);
	    put_pixel(10 + 0xF - i, 10 + i);
	//}
    }
    getch();

    asm {
        mov ax, 0x03
        int 0x10  // 화면을 텍스트 모드로 전환
    }
}
