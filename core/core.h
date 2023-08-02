#pragma once

// incremented by the hatari VBL handler, used to exit the CPU emulation loop after 1 frame
extern int core_frame_advance;

// 0=0RGB1555, 1=XRGB8888, 2=RGB565
extern int core_pixel_format;

// print to Libretro debug log
extern void core_debug_msg(const char* msg);
extern void core_debug_int(const char* msg, int num); // msg followed by num
extern void core_debug_hex(const char* msg, unsigned int num); // msg followed by hex

// send video frame update to core
extern void core_video_update(const void* data, int width, int height, int pitch);

// hatari exports
extern int main_init(int argc, char *argv[]);
extern int main_deinit(void);
extern void m68k_go_frame(void);
extern void m68k_go_quit(void);

