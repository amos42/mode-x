#ifndef __MODE_X_H_
#define __MODE_X_H_

#ifdef __cplusplus
extern "C" {
#endif


extern char* _vram;
extern int _vwidth;
extern int _vheight;


void set_screen_mode(int mode);
void set_crtc_to_240_lines();
void turn_off_chain4();
void set_bank_mask(int mask);
void select_bank(int bank);
void set_write_mode(int mode);
void set_write_func(int op_mode);
void wait_vsync();
void set_start_addr(unsigned int start_offs);
void set_start_addr_hi(unsigned char start_offs_hi);
void set_virtual_width(int width);
void set_bitmask(unsigned char mask);

void set_palette(int idx, unsigned char red, unsigned char green, unsigned char blue);
void set_palette_multi(unsigned char* pal, int start, int count);

void clear_screen( char *vram, char color );
void send_image_to_vram( char *vdes, int offs, char* img, int width, int height );
void put_image_to_vram( char *vram, int x, int y, char* img, int width, int height );
void copy_vram_to_vram_image( char *vdes, char *vsrc, int width, int height );
void put_vram_to_vram_image( char *vram, int x, int y, char *vsrc, int width, int height );

char* make_shift_image(char* img, int width, int height, int* new_width, int* new_height);
void put_sprite_to_vram( char *vram, int x, int y, char* img, int width, int height, unsigned char* mask );
void put_sprite_to_vram2(char *vram, int x, int y, char* imgs[4], int width, int height, unsigned char* mask[4]);

void put_font_to_vram(char *vram, unsigned char* font, int x, int y, int width, int height, char color);

void draw_box(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color);
void draw_bar(char* buf, int vwidth, int x1, int y1, int x2, int y2, char color);
void make_check_screen(char* buf, int vwidth, int width, int height, char color1, char color2, char color3);
unsigned char* make_image_mask(char* img, int img_x, int img_y, char trans_color);


#ifdef __cplusplus
}
#endif

#endif // __MODE_X_H_
