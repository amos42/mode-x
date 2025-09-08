#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <malloc.h>
// #include "GAME13H.H"

#define GC_INDEX   0x3CE
#define GC_DATA    0x3CF
#define   GC_SET_RESET              0x00
#define   GC_ENABLE_SET_RESET       0x01
#define   GC_FUNC                   0x03
#define   GC_READ_MAP_SELECT        0x04
#define   GC_MODE                   0x05

#define SA_INDEX   0x3C4
#define SA_DATA    0x3C5
#define   SA_PLANE_MASK             0x02
#define   SA_MEMORY_MODE            0x04

#define DAC_INDEX  0x3C8
#define DAC_DATA   0x3C9

#define CRTC_INDEX 0x3D4
#define CRTC_DATA  0x3D5
#define   CRTC_START_ADDR_HI        0x0C
#define   CRTC_START_ADDR_LO        0x0D
#define   CRTC_VERT_DISP_ENABLE_END 0x12
#define   CRTC_OFFSET               0x13
#define   CRTC_UNDERLINE_LOCATTION  0x14
#define   CRTC_MODE_CONTROL         0x17


#define PAGE_HORIZ_COUNT (2)
#define PAGE_VERT_COUNT (2)

char* vram = (char*)0xA0000000;

void set_screen_mode(int mode) {
    union REGS regs;
    regs.h.ah = 0x00;
    regs.h.al = mode;
    int86(0x10, &regs, &regs);
}

void set_pallete_all(unsigned char* pal)
{
    outportb(DAC_INDEX, 0);
    for (int i = 0; i < 256; i++) {
        outportb(DAC_DATA, *pal++);
        outportb(DAC_DATA, *pal++);
        outportb(DAC_DATA, *pal++);
    }
}

void set_plane_mode()
{
    outportb(SA_INDEX, SA_MEMORY_MODE);
    outportb(SA_DATA, inportb(SA_DATA) & ~8);

    // outportb(CRTC_INDEX, CRTC_UNDERLINE_LOCATTION);
    // outportb(CRTC_DATA, 0x00);
    // outportb(CRTC_INDEX, CRTC_MODE_CONTROL);
    // outportb(CRTC_DATA, 0xa3);
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

void set_virtual_width(int width)
{
    outportb(CRTC_INDEX, CRTC_OFFSET);                    // CRTC Offset 레지스터
    outportb(CRTC_DATA, (width / (4 * 2)) & 0xFF);  // 1바이트 = 4픽셀, 단위는 1/2로 지정
}

void set_crtc_to_240_lines() {
    outportb(CRTC_INDEX, CRTC_VERT_DISP_ENABLE_END);       // Vertical Display End
    outportb(CRTC_DATA, 224);        // 줄 수를 400줄로 축소 (144 * 4 = 576 => 실제 400 비슷한 느낌으로 맞춤)
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

void draw_box(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color)
{
    int i, j;
    for (i = x1; i <= x2; i++) {
        buf[y1 * vwidth + i] = color;
        buf[y2 * vwidth + i] = color;
    }
    for (i = y1 + 1; i < y2; i++) {
        buf[i * vwidth + x1] = color;
        buf[i * vwidth + x2] = color;
    }
}

void draw_bar(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color)
{
    int i, j;
    for (i = y1; i <= y2; i++) {
        for (j = x1; j <= x2; j++) {
            buf[i * vwidth + j] = color;
        }
    }
}

void gen_screen(char* buf, int vwidth, int width, int height, char color1, char color2, char color3)
{
    int i, j;
    for (i = 0; i < height / 8; i++) {
        for (j = 0; j < width / 8; j++) {
            if ((i + j) % 2 == 0) {
                draw_bar(buf, vwidth, j * 8, i * 8, j * 8 + 7, i * 8 + 7, color1);
            }
            else {
                draw_bar(buf, vwidth, j * 8, i * 8, j * 8 + 7, i * 8 + 7, color2);
            }
        }
    }
    draw_box(buf, vwidth, 0, 0, width - 1, height - 1, color3);
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

void put_vram_to_vram(int xpage, int ypage, int s_xpage, int s_ypage, int start_x, int start_y, int s_start_x, int s_start_y, int width, int height)
{
    char* src = vram + s_ypage * (320 / 4 * 200) * PAGE_HORIZ_COUNT + 320 / 4 * s_xpage + s_start_y * (320 / 4) * PAGE_HORIZ_COUNT + s_start_x / 4;
    char* des = vram + ypage * (320 / 4 * 200) * PAGE_HORIZ_COUNT + 320 / 4 * xpage + start_y * (320 / 4) * PAGE_HORIZ_COUNT + start_x / 4;

    set_write_mode(1);

    set_plane_mask(0xf);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 4; j++) {
            int dummy = *(src + j);
            *(des + j) = 0;
        }
        src += 320 / 4 * 2;
        des += 320 / 4 * 2;
    }

    set_write_mode(0);
}

void put_image_to_vram(int xpage, int ypage, char* src, int start_x, int start_y, int width, int height)
{
    char* des0 = vram + ypage * (320 / 4 * 200) * PAGE_HORIZ_COUNT + 320 / 4 * xpage + start_y * (320 / 4) * PAGE_HORIZ_COUNT + start_x / 4;

    for (int k = 0; k < 4; k++) {
        select_plane(k);

        char* des = des0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width / 4; j++) {
                des[j] = src[i * width + j * 4 + k];
            }
            des += 320 / 4 * PAGE_HORIZ_COUNT;
        }
    }
}

void put_image_to_vram_mask(int xpage, int ypage, unsigned char* mask, int start_x, int start_y, int width, int height)
{
    char* des = vram + ypage * (320 / 4 * 200) * PAGE_HORIZ_COUNT + 320 / 4 * xpage + start_y * (320 / 4) * PAGE_HORIZ_COUNT + start_x / 4;

    set_write_mode(2);

    set_plane_mask(0xF);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 4; j += 2) {
            unsigned char m = *mask++;
            des[j] = m;
            des[j + 1] = m >> 4;
        }
        des += 320 / 4 * PAGE_HORIZ_COUNT;
    }

    set_write_mode(0);
}

void put_image_to_vram_mask0(int xpage, int ypage, unsigned char* mask, int start_x, int start_y, int width, int height)
{
    char* des = vram + ypage * (320 / 4 * 200) * PAGE_HORIZ_COUNT + 320 / 4 * xpage + start_y * (320 / 4) * PAGE_HORIZ_COUNT + start_x / 4;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 4; j += 2) {
            unsigned char m = *mask++;
            int k;
            for (k = 0; k < 4; k++) {
                select_plane(k);
                des[j] = ((m >> k) & 0x01) ? 0xF : 0x0;
            }
            m >>= 4;
            for (k = 0; k < 4; k++) {
                select_plane(k);
                des[j + 1] = ((m >> k) & 0x01) ? 0xF : 0x0;
            }
        }
        des += 320 / 4 * PAGE_HORIZ_COUNT;
    }
}

void put_vram_to_vram_sprite(int xpage, int ypage, int s_xpage, int s_ypage, int start_x, int start_y, int width, int height, unsigned char* mask)
{
    char* src = vram + s_ypage * (320 / 4 * 200) * PAGE_HORIZ_COUNT + 320 / 4 * s_xpage;
    char* des = vram + ypage * (320 / 4 * 200) * PAGE_HORIZ_COUNT + 320 / 4 * xpage + start_y * (320 / 4) * PAGE_HORIZ_COUNT + start_x / 4;

    set_write_mode(1);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 4; j += 2) {
            unsigned char m = *mask++;

            int dummy = *(src + j);
            set_plane_mask(m);
            *(des + j) = 0x00;

            dummy = *(src + j + 1);
            set_plane_mask(m >> 4);
            *(des + j + 1) = 0x00;
        }
        src += 320 / 4 * PAGE_HORIZ_COUNT;
        des += 320 / 4 * PAGE_HORIZ_COUNT;
    }

    set_write_mode(0);
}

void set_bitmask(unsigned char mask) {
    outp(GC_INDEX, 0x08);
    outp(GC_DATA, mask);
}


unsigned char font8x8[1][8] = {
    {0x18,0x24,0x24,0x42,0x7E,0x42,0x42,0x00}
};
// 쓰기 모드2 설정
void set_write_mode2() {
    outp(GC_INDEX, 0x05);
    // outp(GC_DATA, inp(GC_DATA) & 0xFC | 0x02); // write mode 2
    outp(GC_DATA, 0x02); // write mode 2
}
// 글자 하나 출력 (8x8)
void draw_char_mode2(int x, int y, char ch, char color) {
    unsigned short offset = (y * 80) + (x >> 2); // 바이트 오프셋

    set_write_mode2();

    for (int row = 0; row < 8; row++) {
        unsigned char fontrow = font8x8[0][row];

        // 비트마스크 = fontrow
        set_bitmask(fontrow);

        // 래치 채우기 (dummy read)
        volatile char dummy = vram[offset];

        // 실제 쓰기: 색상 한 바이트
        vram[offset] = color;

        offset += 320 / 4 * PAGE_HORIZ_COUNT; // 다음 라인
    }
}


void put_font_to_vram(int xpage, int ypage, unsigned char* font, int start_x, int start_y, int width, int height, char color)
{
    char* des = vram + ypage * (320 / 4 * 200) * PAGE_HORIZ_COUNT + 320 / 4 * xpage + start_y * (320 / 4) * PAGE_HORIZ_COUNT + start_x / 4;

    // set_write_mode(2);
    set_write_mode2();
    // set_write_func(1);

    // outportb(GC_INDEX, GC_ENABLE_SET_RESET); // Set/Reset
    // outportb(GC_DATA, 0xf);  // Set color (0-15)
    // outportb(GC_INDEX, GC_SET_RESET); // Set/Reset
    // outportb(GC_DATA, 0xfa);  // Set color (0-15)

    set_plane_mask(0xF);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 4; j += 2) {
            unsigned char m = *font++;
            // set_plane_mask(m);
    // outportb(GC_INDEX, 8); // Set/Reset
    // outportb(GC_DATA, m);  // Set color (0-15)
            set_bitmask(m);

            volatile char dummy = des[j];
            des[j] = color;
            // duymmy = des[j+1];
            // set_plane_mask((m >> 4) & 0xF);
    // outportb(GC_INDEX, 8); // Set/Reset
    // outportb(GC_DATA, m >> 4);  // Set color (0-15)
    //         des[j+1] = color;
        }
        des += 320 / 4 * PAGE_HORIZ_COUNT;
    }

    set_write_mode(0);
}

void main()
{
    int i;
    unsigned short img_x, img_y;
    char* img;
    unsigned char* img_mask;
    unsigned char palette[256 * 3];

    // FILE* fp = fopen("hose.img", "rb");
    // fseek(fp, 16 + 1 + 1 + 64 * 4, SEEK_SET);
    // unsigned short img_x, img_y;
    // fread(&img_x, 2, 1, fp);
    // fread(&img_y, 2, 1, fp);
    // unsigned char* img = (unsigned char*)malloc(img_x * img_y);
    // fread(img, img_x * img_y, 1, fp);
    // fclose(fp);

    // char str[256];
    // sprintf(str, "I%dx%d.img", img_x, img_y);
    // fp = fopen(str, "wb");
    // fwrite(img, img_x * img_y, 1, fp);
    // fclose(fp);


    FILE* fp = fopen("hose.img", "rb");
    fseek(fp, 16 + 1 + 1 + 64 * 4, SEEK_SET);
    fread(&img_x, 2, 1, fp);
    fread(&img_y, 2, 1, fp);
    img = (char*)malloc(img_x * img_y);
    fread(img, img_x * img_y, 1, fp);
    fclose(fp);
    // gen image mask
    img_mask = gen_image_mask(img, img_x, img_y, 0xFF);

    fp = fopen("pal.bin", "rb");
    fread(palette, 256 * 3, 1, fp);
    fclose(fp);

    char* p1 = (char*)malloc(64000);
    gen_screen(p1, 320, 320, 200, 0x07, 0x08, 0x01);

    char* p2 = (char*)malloc(64000);
    gen_screen(p2, 320, 320, 200, 0x03, 0x0B, 0x04);

    set_screen_mode(0x13);

    set_plane_mode();
    set_virtual_width(320 * PAGE_HORIZ_COUNT);
    //  set_crtc_to_240_lines();

    set_pallete_all(palette);
    put_image_to_vram(0, 0, p1, 0, 0, 320, 200);
    put_image_to_vram(1, 0, p2, 0, 0, 320, 200);
    put_vram_to_vram(0, 1, 1, 0, 0, 0, 0, 0, 320, 200);
    put_vram_to_vram(1, 1, 0, 0, 0, 0, 0, 0, 320, 200);

    put_image_to_vram(1, 0, img, 0, 0, img_x, img_y);
    // int idx=0;
    //for(i=0;i<img_y;i++)
    //    for(int j=0;j<img_x/8;j++)
    //        img_mask[idx++]=i % 2 ? 0xAA : 0x55;
    put_image_to_vram(0, 0, img, 16, 16, img_x, img_y);
    put_image_to_vram_mask(0, 0, img_mask, 16 + 120, 16 + 100, img_x, img_y);
    // put_vram_to_vram_sprite(0, 0, 1, 0, 16+48, 16+96, img_x, img_y, img_mask);

    // put_font_to_vram(0, 0, img_mask, 16+120, 16, img_x, img_y, 0x1);

    // draw_char_mode2(40, 40, 'A', 12);

    put_font_to_vram(0, 0, img_mask, 16 + 120, 16, img_x, img_y, 0x1);

    getch();

    // for (i = 0; i <= 320 / 4; i++) {
    //     set_start_index(i);
    //     delay(5);
    // }

    // getch();

    // for (i = 0; i <= 200; i++) {
    //     set_start_index(320 / 4 + 320 / 4 * 2 * i);
    //     delay(2);
    // }

    // getch();

    // for (i = 320 / 4; i >= 0; i--) {
    //     set_start_index(i + 320 / 4 * 2 * 200);
    //     delay(5);
    // }

    // getch();

    // for (i = 200; i >= 0; i--) {
    //     set_start_index(320 / 4 * 2 * i);
    //     delay(2);
    // }

    // getch();

    set_screen_mode(0x03);

    free(p2);
    free(p1);
    free(img);
    free(img_mask);
}
