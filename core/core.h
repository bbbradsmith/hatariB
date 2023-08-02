#pragma once

// incremented by the hatari VBL handler, used to exit the CPU emulation loop after 1 frame
extern int core_frame_advance;

// print to Libretro debug log
extern void core_debug_log(const char* msg);

// hatari exports
extern int main_init(int argc, char *argv[]);
extern int main_deinit(void);
extern void m68k_go_frame(void);
extern void m68k_go_quit(void);

