// mode_x_bitmap_font.c
// 16비트 DOS용 C 코드 (Turbo C/Borland C/Watcom C 등) 예제
// VGA Mode X에서 쓰기 모드2를 사용해 8x8 비트맵 폰트를 출력
// 컴파일 후 DOSBox에서 실행 가능

#include <dos.h>
#include <conio.h>
//#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include "../src/VGA_REG.H"


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

void set_screen_mode(int mode)
{
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}

// Mode X 초기화 (chain-4 끄고 320x240)
void mode_x_init() {
    // Sequencer: disable chain-4
    // outp(VGA_SEQ_INDEX, 0x04);
    // outp(VGA_SEQ_DATA, 0x06);

    // none-chain-4 모드
    outportb(SA_INDEX, SA_MEMORY_MODE);
    // outportb(SA_DATA, inportb(SA_DATA) & ~(1 << 3));
    outportb(SA_DATA, 0x06);

    // 스캔 주기 60Hz
    outportb(MISC_OUTPUT, 0xE3);

    // 1byte 전송 모드
    outportb(CRTC_INDEX, CRTC_UNDERLINE_LOCATTION);
    outportb(CRTC_DATA, 0x00);
    // outportb(CRTC_DATA, inportb(CRTC_DATA) & ~(1 << 5) | (1 << 6));
    outportb(CRTC_INDEX, CRTC_MODE_CONTROL);
    outportb(CRTC_DATA, 0xE3);
    // outportb(CRTC_DATA, inportb(CRTC_DATA) & ~((1 << 6) | (1 << 3)));

    // CRTC 수직 해상도 관련 레지스터의 조작을 막는다.
    outportb(CRTC_INDEX, CRTC_VERTICAL_SYNC_END);
    outportb(CRTC_DATA, inportb(CRTC_DATA) & 0x7F);


    // Graphics Controller: set write mode 2 later
    // outp(VGA_GC_INDEX, 0x05);
    // outp(VGA_GC_DATA, 0x00);

    // CRTC: 늘려서 240 라인 (간단히 일부만 조정)
    // outp(VGA_CRTC_INDEX, 0x09);
    // outp(VGA_CRTC_DATA, 0x40);
}

void set_write_mode(int mode)
{
    outportb(GC_INDEX, GC_MODE);
    outportb(GC_DATA, inportb(GC_DATA) & 0xFC | mode);
    // outportb(GC_DATA, mode);
}

void set_read_mode(int mode)
{
    outportb(GC_INDEX, GC_MODE);
    outportb(GC_DATA, inportb(GC_DATA) & 0x9F | mode << 5);
}

// 쓰기 모드2 설정
// void set_write_mode2() {
//     outp(VGA_GC_INDEX, 0x05);
//     outp(VGA_GC_DATA, 0x02); // write mode 2
// }

// 비트마스크 레지스터 설정
void set_bitmask(unsigned char mask) {
    outp(VGA_GC_INDEX, 0x08);
    outp(VGA_GC_DATA, mask);
}

unsigned char* gen_image_mask(char* img, int img_x, int img_y, char trans_color)
{
    unsigned char* img_mask = (unsigned char*)malloc(img_x / 8 * img_y);
    for (int i = 0; i < img_y; i++) {
        for (int j = 0; j < img_x / 8; j++) {
            unsigned char m = 0;
            for (int k = 0; k < 8; k++) {
                if (img[i * img_x + j * 8 + k] != trans_color) {
                    m |= (1 << (7 - k));
                    //m |= (1 << k);
                }
            }
            img_mask[i * (img_x / 8) + j] = m;
        }
    }
    return img_mask;
}

// 글자 하나 출력 (8x8)
void draw_char_mode2(int x, int y, char ch, char color)
{
    unsigned short offset = (y * 80) + (x >> 2); // 바이트 오프셋
    unsigned char* vram = (unsigned char *)MK_FP(VIDEO_SEG, 0);

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


void draw_mask_mode2(int x, int y, unsigned char *mask, int width, int height, char color)
{
    unsigned short offset = (y * 80) + (x >> 2); // 바이트 오프셋
    unsigned char* vram = (unsigned char *)MK_FP(VIDEO_SEG, 0);

    set_write_mode(2);
    // set_write_func(1);

    // outportb(GC_INDEX, GC_ENABLE_SET_RESET); // Set/Reset
    // outportb(GC_DATA, 0xf);  // Set color (0-15)
    // outportb(GC_INDEX, GC_SET_RESET); // Set/Reset
    // outportb(GC_DATA, 0xfa);  // Set color (0-15)

    // set_plane_mask(0xF);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 4; j += 2) {
            unsigned char m = *mask++;
            set_bitmask(m);

            volatile char dummy = vram[offset+j];
            vram[offset+j] = color;
        }
        offset += 80; // 다음 라인
    }
}


int main()
{
    int i;
    unsigned short img_x, img_y;
    char* img;
    unsigned char* img_mask;

    FILE* fp = fopen("sonic.img", "rb");
    fread(&img_x, 2, 1, fp);
    fread(&img_y, 2, 1, fp);
    img = (char*)malloc(img_x * img_y);
    fread(img, img_x * img_y, 1, fp);
    fclose(fp);
    // gen image mask
    img_mask = gen_image_mask(img, img_x, img_y, 0xFF);

    set_screen_mode(0x13);
    mode_x_init();

    draw_char_mode2(40, 40, 'A', 12);
    draw_char_mode2(60, 40, 'A', 14);
    draw_mask_mode2(100, 40, img_mask, img_x, img_y, 1);

    getch();
    
    set_screen_mode(0x03);
    return 0;
}
