#pragma once

// internal core stuffs that needs to be shared between core modules,
// but not with Hatari. Avoids a full rebuild for each change.

#define   CORE_ARRAY_SIZE(a_)   (sizeof(a_)/sizeof(a_[0]))

// core.c
extern retro_environment_t environ_cb;
extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;
extern retro_log_printf_t retro_log;
extern int core_video_aspect_mode;
extern bool core_video_changed;
extern bool core_option_hard_reset;
extern bool core_serialize_write; // current serialization direction

// core_file.c
extern void strcpy_trunc(char* dest, const char* src, unsigned int len);
extern void strcat_trunc(char* dest, const char* src, unsigned int len);

extern uint8_t* core_read_file(const char* filename, unsigned int* size_out);
extern bool core_write_file(const char* filename, unsigned int size, const uint8_t* data);
extern uint8_t* core_read_file_system(const char* filename, unsigned int* size_out);
extern uint8_t* core_read_file_save(const char* filename, unsigned int* size_out);
extern bool core_write_file_save(const char* filename, unsigned int size, const uint8_t* data);
const char* get_temp_fn(); // gets the last temporary path created for a save/system read or write (use carefully)

// direct file access
// file access types:
//   read-only (rb), write-truncate (wb), read-write (wb+), read-write-truncate (rb+)
#define CORE_FILE_READ       0
#define CORE_FILE_WRITE      1
#define CORE_FILE_REVISE     2
#define CORE_FILE_TRUNCATE   3
struct stat; // defined where used
struct dirent; // defined where used
extern void* core_file_open(const char* path, int access);
extern void* core_file_open_system(const char* path, int access);
extern void* core_file_open_save(const char* path, int access);
extern bool core_file_exists(const char* path); // returns true if file exists and is not a directory (and is read or writable)
extern bool core_file_save_exists(const char* filename);
extern void core_file_close(void* file);
extern int core_file_seek(void* file, int64_t offset, int dir);
extern int64_t core_file_read(void* buf, int64_t len, void* file);
extern int64_t core_file_write(const void* buf, int64_t len, void* file);
extern int core_file_flush(void* file);
extern int core_file_remove(const char* path);
extern int core_file_rename(const char* old_path, const char* new_path);
extern int core_file_stat(const char* path, struct stat* fs);
extern void* core_file_opendir(const char* path);
extern struct dirent* core_file_readdir(void* dir);
extern int core_file_closedir(void* dir);

extern void core_file_set_environment(retro_environment_t cb); // scans system/ folder, includes "tos.img" and everything in"hatarib/" (non-recursive)
extern int core_file_system_count(); // number of files found
extern const char* core_file_system_filename(int index); // file list, first is "tos.img" if it exists, and "hatarib/" files follow (with "hatarib/ prefix)
extern int core_file_system_dir_count();
extern const char* core_file_system_dirname(int index); // "hatarib/dir"
extern const char* core_file_system_dirlabel(int index); // "dir/"

// core_disk.c
extern void core_disk_set_environment(retro_environment_t cb);
extern void core_disk_init(void);
extern void core_disk_load_game(const struct retro_game_info *game);
extern void core_disk_unload_game(void);
extern void core_disk_reindex(void); // call after loading a savestate to rebuild disk cache indices
extern void core_disk_drive_toggle(void);
extern void core_disk_drive_reinsert(void); // used after cold reboot

// simple file save, as a complete buffer
// after saving, the image cached in core_disk.c will be updated (if the file is cached),
// and if core_owns_data it will just take over the pointer instead of copying it.
 extern bool core_disk_save(const char* filename, uint8_t* data, unsigned int size, bool core_owns_data);

// advanced file save, as serial writes
extern void* core_disk_save_open(const char* filename);
extern void core_disk_save_close(void* file, bool success); // if !success, will delete the file after closing
extern bool core_disk_save_write(const uint8_t* data, unsigned int size, void* file);
extern bool core_disk_save_exists(const char* filename);

extern bool core_disk_enable_b;
extern bool core_disk_enable_save;

// core_config.c
extern void core_config_set_environment(retro_environment_t cb); // call after core_disk_set_environment (which scans system folder for TOS etc)
extern void core_config_apply(void);

// core_input.c
extern void core_input_set_environment(retro_environment_t cb);
extern void core_input_init(void);
extern void core_input_update(void); // call in retro_run, polls Libretro inputs and translates to events for hatari
extern void core_input_post(void); // call to force hatari to process the input queue
extern void core_input_finish(void); // call at end of retro_run
extern void core_input_serialize(void);

extern bool core_mouse_port;
extern bool core_host_keyboard;
extern bool core_host_mouse;
extern int core_autofire;
extern int core_stick_threshold;
extern int core_mouse_speed;
extern int core_mouse_dead;

// input mapping index

extern int core_joy_port_map[4];

#define CORE_INPUT_STICK_DPAD      0
#define CORE_INPUT_STICK_L         1
#define CORE_INPUT_STICK_R         2
extern int core_stick_map[4][3];

#define CORE_INPUT_BUTTON_A        0
#define CORE_INPUT_BUTTON_B        1
#define CORE_INPUT_BUTTON_X        2
#define CORE_INPUT_BUTTON_Y        3
#define CORE_INPUT_BUTTON_SELECT   4
#define CORE_INPUT_BUTTON_START    5
#define CORE_INPUT_BUTTON_L1       6
#define CORE_INPUT_BUTTON_R1       7
#define CORE_INPUT_BUTTON_L2       8
#define CORE_INPUT_BUTTON_R2       9
#define CORE_INPUT_BUTTON_L3       10
#define CORE_INPUT_BUTTON_R3       11
extern int core_button_map[4][12];

#define CORE_INPUT_OSKEY_CONFIRM   0
#define CORE_INPUT_OSKEY_CANCEL    1
#define CORE_INPUT_OSKEY_SHIFT     2
#define CORE_INPUT_OSKEY_POS       3
#define CORE_INPUT_OSKEY_MOVE      4
extern int core_oskey_map[4][5];
