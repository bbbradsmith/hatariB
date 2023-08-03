#pragma once

// 0=0RGB1555, 1=XRGB8888, 2=RGB565
extern int core_pixel_format;

// print to Libretro log
extern void core_debug_msg(const char* msg);
extern void core_debug_int(const char* msg, int num); // msg followed by num
extern void core_debug_hex(const char* msg, unsigned int num); // msg followed by hex
extern void core_error_msg(const char* msg);

// send video/audio frame update to core
extern void core_video_update(void* data, int width, int height, int pitch);
extern void core_audio_update(const int16_t data[][2], int index, int length);
extern void core_set_fps(int rate);
extern void core_set_samplerate(int rate);

// input translation
typedef union SDL_Event* core_event_t;
extern int core_poll_event(core_event_t event);
extern int core_poll_joy_fire(int port);
extern int core_poll_joy_stick(int port);

// hatari exports
extern int main_init(int argc, char *argv[]);
extern int main_deinit(void);
extern void m68k_go_frame(void);
extern void m68k_go_quit(void);
