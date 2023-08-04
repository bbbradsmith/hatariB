#pragma once

// 0=0RGB1555, 1=XRGB8888, 2=RGB565
extern int core_pixel_format;

// will exit from m68k_go_frame before running CPU cycles if true
extern bool core_init_return;

// print to Libretro log
extern void core_debug_msg(const char* msg);
extern void core_debug_int(const char* msg, int num); // msg followed by num
extern void core_debug_hex(const char* msg, unsigned int num); // msg followed by hex
extern void core_error_msg(const char* msg);
extern void core_debug_bin(const char* data, int len, int offset);

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

// in-memory savestate
extern void core_snapshot_open(void);
extern void core_snapshot_close(void);
extern void core_snapshot_read(char* buf, int len);
extern void core_snapshot_write(const char* buf, int len);
extern void core_snapshot_seek(int pos);
// bi-directional serialization helpers
extern void core_serialize_uint8(uint8_t *x);
extern void core_serialize_int32(int32_t *x);

// hatari exports
extern int main_init(int argc, char *argv[]);
extern int main_deinit(void);
extern void m68k_go_frame(void);
extern void m68k_go_quit(void);
