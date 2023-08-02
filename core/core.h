#pragma once

// print to Libretro debug log
extern void core_debug_log(const char* msg);

// hatari exports
extern int main_init(int argc, char *argv[]);
extern int main_deinit(void);
