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


#ifdef __cplusplus
}
#endif

#endif // __MODE_X_H_
