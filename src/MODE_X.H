#ifndef __MODE_X_H_
#define __MODE_X_H_

#ifdef __cplusplus
extern "C" {
#endif


extern char* _vram;
extern int _vwidth;
extern int _vheight;

void set_crtc_to_240_lines();
void turn_off_chain4();
void set_bank_mask(int mask);
void select_bank(int bank);
void set_write_mode(int mode);
void set_write_func(int op_mode);
void wait_vsync();
void set_start_addr(unsigned int start_offs);
void sset_start_addr_hi(unsigned char start_offs_hi);
void set_virtual_width(int width);

void clear_screen( char *vram, char color );
void send_image_to_vram( char *vdes, int offs, char* img, int width, int height );
void put_image_to_vram( char *vram, int x, int y, char* img, int width, int height );

unsigned char* make_image_mask(char* img, int img_x, int img_y, char trans_color);
char* make_shift_image(char* img, int width, int height, int* new_width, int* new_height);
void put_sprite_to_vram( char *vram, int x, int y, char* img, int width, int height, unsigned char* mask );
void put_sprite_to_vram2(char *vram, int x, int y, char* imgs[4], int width, int height, unsigned char* mask[4]);


#ifdef __cplusplus
}
#endif

#endif // __MODE_X_H_
