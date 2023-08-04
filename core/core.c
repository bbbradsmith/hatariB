#include "../libretro/libretro.h"
#include "core.h"
#include <string.h>
#include <stdlib.h>

// large enough for TT high resolution 1280x960 at 32bpp
#define VIDEO_MAX_W   2048
#define VIDEO_MAX_H   1024
#define VIDEO_MAX_PITCH   (VIDEO_MAX_W*4)
// 4 frames of buffer at slowest framerate
#define AUDIO_BUFFER_LEN   (4*2*96000/50)

// header size must accomodate core data before the hatari memory snapshot
// overhead is some safety extra in case hatari wants to increase the snapshot size later
// (doesn't seem to change size without config changes that might need a restart)
// round is an even file size to round up to
#define SNAPSHOT_HEADER_SIZE 1024
#define SNAPSHOT_OVERHEAD    (64 * 1024)
#define SNAPSHOT_ROUND       (64 * 1024)
#define SNAPSHOT_VERSION     1

//
// Libretro
//

static void null_log(enum retro_log_level level, const char* msg, ...) { (void)level; (void)msg; }

retro_environment_t environ_cb;
retro_video_refresh_t video_cb;
retro_audio_sample_t audio_sample_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t input_poll_cb;
retro_input_state_t input_state_cb;
retro_log_printf_t retro_log = null_log;

static bool BOOL_TRUE = true;
static enum retro_pixel_format RPF_0RGB1555 = RETRO_PIXEL_FORMAT_0RGB1555;
static enum retro_pixel_format RPF_XRGB8888 = RETRO_PIXEL_FORMAT_XRGB8888;
static enum retro_pixel_format RPF_RGB565   = RETRO_PIXEL_FORMAT_RGB565;

static struct retro_core_option_definition CORE_OPTIONS[] = {
	// TODO
	{
		"hatarib_option1",
		"Option 1 test",
		"A true/false option",
		{
			{ "true",  "enabled"  },
			{ "false", "disabled" },
			{ NULL, NULL },
		},    "false"
	},
	{
		"hatarib_option2",
		"Option 2 test",
		"A list option",
		{
			{ "0", "list 0" },
			{ "1", NULL },
			{ "3", NULL },
			{ "9", "list 9" },
			{ NULL, NULL },
		},    "3"
	},
	{ NULL, NULL, NULL, {{0}}, NULL },
};

//
// From Hatari
//

extern uint8_t* STRam;
extern uint32_t STRamEnd;
extern int TOS_DefaultLanguage(void);
extern int Reset_Warm(void);
extern int hatari_libretro_save_state(void);
extern void hatari_libretro_restore_state(void);

//
// Core implementation in other files
//

// core_input.c
extern void core_input_init(void); // call in retro_init
extern void core_input_update(void); // call in retro_run, polls Libretro inputs and translates to events for hatari
extern void core_input_post(void); // call to force hatari to process the input queue
extern void core_input_finish(void); // call at end of retro_run
extern void core_input_serialize(void); // savestate save/load

//
// Available to Hatari
//

// external
int core_pixel_format = 0;
bool core_init_return = false;

// internal
void* core_video_buffer = NULL;
int16_t core_audio_buffer[AUDIO_BUFFER_LEN];
int core_video_w = 640;
int core_video_h = 400;
int core_video_pitch = 640*4;
int core_video_fps = 50;
int core_audio_samplerate = 48000;
int core_audio_samples_pending = 0;
bool core_video_changed = false;
bool core_rate_changed = false;

void core_debug_msg(const char* msg)
{
	retro_log(RETRO_LOG_DEBUG,"%s\n",msg);
}

void core_debug_int(const char* msg, int num)
{
	retro_log(RETRO_LOG_DEBUG,"%s%d\n",msg,num);
}

void core_debug_hex(const char* msg, unsigned int num)
{
	retro_log(RETRO_LOG_DEBUG,"%s%08X\n",msg,num);
}

void core_error_msg(const char* msg)
{
	retro_log(RETRO_LOG_ERROR,"%s\n",msg);
}

void core_debug_bin(const char* data, int len, int offset)
{
	#define DEBUG_BIN_COLUMNS 32
	static char line[3+(DEBUG_BIN_COLUMNS*4)];
	static const char HEX[] = "0123456789ABCDEF";
	static const int cols = DEBUG_BIN_COLUMNS;
	retro_log(RETRO_LOG_DEBUG,"core_debug_bin(%p,%d,%x)\n",data,len,offset);
	for (; len > 0; data += cols, offset += cols, len -= cols)
	{
		// fill line with spaces, terminal zero
		memset(line,' ',sizeof(line));
		line[sizeof(line)-1] = 0;
		// fill data
		for (int i=0; i<cols && i<len ; ++i)
		{
			char b = data[i];
			int shift = (i >= (cols/2)) ? 1 : 0; // extra space mid-row
			int pos = shift + (i*3);
			// hex
			line[pos+0] = HEX[(b>>4)&0xF];
			line[pos+1] = HEX[b&0xF];
			// ascii
			pos = shift + i + 2 + (cols*3);
			line[pos] = '.'; // default if not printable
			if (b >= 0x20 && b < 0x7F) line[pos] = b;
		}
		retro_log(RETRO_LOG_DEBUG,"%010X: %s\n",offset,line);
	}
}

void core_video_update(void* data, int w, int h, int pitch)
{
	if (w > VIDEO_MAX_W) w = VIDEO_MAX_W;
	if (h > VIDEO_MAX_H) w = VIDEO_MAX_H;
	if (pitch > VIDEO_MAX_PITCH) w = VIDEO_MAX_PITCH;
	core_video_buffer = data;
	if (w != core_video_w) { core_video_w = w; core_video_changed = true; }
	if (h != core_video_h) { core_video_h = h; core_video_changed = true; }
	core_video_pitch = pitch;
}

void core_audio_update(const int16_t data[][2], int index, int length)
{
	int pos = core_audio_samples_pending;
	int len = length * 2;
	int max = AUDIO_BUFFER_LEN - pos;
	if (len > max) len = max;
	core_audio_samples_pending += len;
	int l2 = len / 2;
	for (int i=0; i<l2; ++i)
	{
		core_audio_buffer[pos+0] = data[index+i][0];
		core_audio_buffer[pos+1] = data[index+i][1];
		pos += 2;
	}
}

void core_set_fps(int rate)
{
	if (rate != core_video_fps) core_rate_changed = true;
	core_video_fps = rate;
}

void core_set_samplerate(int rate)
{
	if (rate != core_audio_samplerate) core_rate_changed = true;
	core_audio_samplerate = rate;
}

// memory snapshot simulated file

char* snapshot_buffer = NULL;
int snapshot_pos = 0;
int snapshot_max = 0;
int snapshot_size = 0;
bool snapshot_error = false;

static void core_snapshot_open_internal(void)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_open_internal()\n");
	snapshot_pos = 0;
	snapshot_max = 0;
	snapshot_error = false;
}

void core_snapshot_open(void)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_open()\n");
	snapshot_pos = SNAPSHOT_HEADER_SIZE;
	snapshot_max = snapshot_pos;
}

void core_snapshot_close(void)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_close() max: %X = %d\n",snapshot_max,snapshot_max);
}

void core_snapshot_read(char* buf, int len)
{
	int copy_len = len;
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_read(%p,%d) @ %X\n",buf,len,snapshot_pos);
	if ((snapshot_pos+len) > snapshot_size)
	{
		if (!snapshot_error)
		{
			retro_log(RETRO_LOG_ERROR,"core_snapshot_read out of bounds: %X + %d > %d\n",snapshot_pos,len,snapshot_size);
		}
		copy_len = snapshot_size - snapshot_pos;
		if (copy_len < 0) copy_len = 0;
		snapshot_error = true;
	}
	memcpy(buf,snapshot_buffer+snapshot_pos,copy_len);
	snapshot_pos += len;
	if (snapshot_pos > snapshot_max) snapshot_max = snapshot_pos;
}

void core_snapshot_write(const char* buf, int len)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_write(%p,%d) @ %X\n",buf,len,snapshot_pos);
	if (snapshot_buffer)
	{
		if ((snapshot_pos + len) > snapshot_size)
		{
			if (!snapshot_error)
			{
				retro_log(RETRO_LOG_ERROR,"core_snapshot_write: snapshot_size estimate too small (%d), data loss.\n",snapshot_size);
			}
			len = snapshot_size - snapshot_pos;
			if (len < 0) len = 0;
			snapshot_error = true;
		}
		memcpy(snapshot_buffer+snapshot_pos,buf,len);
	}
	snapshot_pos += len;
	if (snapshot_pos > snapshot_max) snapshot_max = snapshot_pos;
}

void core_snapshot_seek(int pos)
{
	//retro_log(RETRO_LOG_DEBUG,"core_snapshot_seek(%X)\n",pos);
	snapshot_pos = SNAPSHOT_HEADER_SIZE + pos;
	if (snapshot_pos > snapshot_max) snapshot_max = snapshot_pos;
}

static void snapshot_buffer_prepare(size_t size)
{
	if (size > snapshot_size)
	{
		retro_log(RETRO_LOG_ERROR,"serialize size %d > snapshot_size %d: enlarging.\n",size,snapshot_size);
		free(snapshot_buffer); snapshot_buffer = NULL;
		snapshot_size = size;
	}
	if (snapshot_buffer == NULL) snapshot_buffer = malloc(snapshot_size);
	memset(snapshot_buffer,0,snapshot_size);
}

static bool core_serialize_write = false;

static void core_serialize_internal(void* x, size_t size)
{
	if (core_serialize_write) core_snapshot_write(x,size);
	else                      core_snapshot_read( x,size);
}

void core_serialize_uint8(uint8_t *x) { core_serialize_internal(x,sizeof(uint8_t)); }
void core_serialize_int32(int32_t*x) { core_serialize_internal(x,sizeof(int32_t)); }

static bool core_serialize(bool write)
{
	int32_t result = 0;
	core_serialize_write = write;
	core_snapshot_open_internal();

	result = SNAPSHOT_VERSION;
	core_serialize_int32(&result);
	if (result != SNAPSHOT_VERSION)
	{
		retro_log(RETRO_LOG_ERROR,"savestate version does not match SNAPSHOT_VERSION (%d != %d)\n",result,SNAPSHOT_VERSION);
		return false;
	}
	core_input_serialize();
	// TODO comment this out
	retro_log(RETRO_LOG_DEBUG,"core_serialize header: %d <= %d\n",snapshot_pos,SNAPSHOT_HEADER_SIZE);
	if (snapshot_pos > SNAPSHOT_HEADER_SIZE)
		retro_log(RETRO_LOG_ERROR,"core_serialize header too large! %d > %d\n",snapshot_pos,SNAPSHOT_HEADER_SIZE);

	result = 0;
	if (write) result = hatari_libretro_save_state();
	else                hatari_libretro_restore_state();

	//retro_log(RETRO_LOG_DEBUG,"core_serialized: %d of %d used\n",snapshot_max,snapshot_size);
	if (result != 0)
	{
		snapshot_error = true;
		retro_log(RETRO_LOG_ERROR,"hatari state save/restore returned with error.\n");
	}
	return !snapshot_error;
}

//
// Libretro Exports
//

RETRO_API void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;

	// core options
	{
		unsigned version = 0;
		if (cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && (version >= 1))
			cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, CORE_OPTIONS);
	}

	// other capabilities
	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &BOOL_TRUE); // allow boot with no disks
	//RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE
	//RETRO_ENVIRONMENT_GET_MIDI_INTERFACE
	// TODO
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_sample_cb = cb;
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

RETRO_API void retro_init(void)
{
	const char* argv[1] = {""};

	// connect log interface
	{
		struct retro_log_callback log_cb;
		if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_cb))
			retro_log = log_cb.log;
		else
			retro_log = null_log;
		retro_log(RETRO_LOG_INFO,"retro_init()\n");
	}

	// try to get the best pixel format we can
	{
		const char* PIXEL_FORMAT_NAMES[3] = { "0RGB1555", "XRGB8888", "RGB565" };
		core_pixel_format = 0;
		if      (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &RPF_XRGB8888)) core_pixel_format = 1;
		else if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &RPF_RGB565  )) core_pixel_format = 2;
		else if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &RPF_0RGB1555)) core_pixel_format = 0;
		// TODO allow configuration to override choice here?
		retro_log(RETRO_LOG_INFO,"Pixel format: %s\n",PIXEL_FORMAT_NAMES[core_pixel_format]);
	}

	// initialize input translation
	core_input_init();

	main_init(1,(char**)argv); // TODO how are paths affected?

	// this will be fetched and applied via retro_get_system_av_info before the first frame begins
	core_video_changed = false;
	core_rate_changed = false;
}

RETRO_API void retro_deinit(void)
{
	retro_log(RETRO_LOG_INFO,"retro_deinit()\n");

	m68k_go_quit();
	main_deinit();
}

RETRO_API unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
	info->library_name = "hatariB";
	info->library_version = "v0 experimental prototype"; // TODO
	info->valid_extensions = "st|msa|img|zip|stx|dim|ipf|m3u"; // TODO
	info->need_fullpath = true; // TODO we want to save floppy overlays instead of writing back, right? treat them as ROM (fullpath irrelevant)
	info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
	retro_log(RETRO_LOG_INFO,"retro_get_system_av_info()\n");

	info->geometry.base_width = core_video_w;
	info->geometry.base_height = core_video_h;
	info->geometry.max_width = VIDEO_MAX_W;
	info->geometry.max_height = VIDEO_MAX_H;
	info->geometry.aspect_ratio = 0; // TODO computed from user PAR setting? (Color Monitor, Monochrome Monitor, NTSC TV, PAL TV, 1:1)
	info->timing.fps = core_video_fps;
	info->timing.sample_rate = core_audio_samplerate;

	retro_log(RETRO_LOG_INFO," geometry.base_width = %d\n",info->geometry.base_width);
	retro_log(RETRO_LOG_INFO," geometry.base_height = %d\n",info->geometry.base_height);
	retro_log(RETRO_LOG_INFO," geometry.aspect_radio = %f\n",info->geometry.aspect_ratio);
	retro_log(RETRO_LOG_INFO," timing.fps = %f\n",info->timing.fps);
	retro_log(RETRO_LOG_INFO," timing.sample_rate = %f\n",info->timing.sample_rate);
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

RETRO_API void retro_reset(void)
{
	Reset_Warm();
}

RETRO_API void retro_run(void)
{
	// poll input, generate event queue for hatari
	core_input_update();

	// force hatari to process the input queue before each frame starts
	core_input_post();

	// run one frame
	m68k_go_frame();

	// send video
	if (core_rate_changed)
	{
		retro_log(RETRO_LOG_INFO,"core_rate_changed\n");
		struct retro_system_av_info info;
		retro_get_system_av_info(&info);
		environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
		core_rate_changed = false;
		core_video_changed = false;
	}
	else if (core_video_changed)
	{
		retro_log(RETRO_LOG_INFO,"core_video_changed\n");
		struct retro_system_av_info info;
		retro_get_system_av_info(&info);
		environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info);
		core_video_changed = false;
	}
	video_cb(core_video_buffer,core_video_w,core_video_h,core_video_pitch);

	// send audio
	if (core_audio_samples_pending > 0)
	{
		unsigned int len = core_audio_samples_pending;
		if (len > AUDIO_BUFFER_LEN) len = AUDIO_BUFFER_LEN;
		audio_batch_cb(core_audio_buffer, len/2);
		core_audio_samples_pending = 0;
	}

	// event queue end of frame
	core_input_finish();
}

RETRO_API size_t retro_serialize_size(void)
{
	return snapshot_size;
}

RETRO_API bool retro_serialize(void *data, size_t size)
{
	//retro_log(RETRO_LOG_DEBUG,"retro_serialize(%p,%d)\n",data,size);
	snapshot_buffer_prepare(size);
	if (core_serialize(true))
	{
		memcpy(data,snapshot_buffer,size);
		//core_debug_bin(data,size,0);
		return true;
	}
	return false;
}

RETRO_API bool retro_unserialize(const void *data, size_t size)
{
	//retro_log(RETRO_LOG_DEBUG,"retro_unserialize(%p,%z)\n",data,size);
	//core_debug_bin(data,size,0);
	snapshot_buffer_prepare(size);
	memcpy(snapshot_buffer,data,size);
	if (core_serialize(false))
	{
		return true;
	}
	return false;
	// TODO seems to hang after this, do we need an unpause or something? (alert dialog was cancelling things!)
	// also: should we call the event loop handler at the end run frame just to make sure it's always clear?
}

RETRO_API void retro_cheat_reset(void)
{
	// no cheat mechanism available
}

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	// no cheat mechanism available
	(void)index;
	(void)enabled;
	(void)code;
}

RETRO_API bool retro_load_game(const struct retro_game_info *game)
{
	// TODO load the game?

	// finish initialization of the CPU
	core_init_return = true;
	m68k_go_frame();
	core_init_return = false;

	// create a dummy savestate to measure the needed snapshot size
	free(snapshot_buffer); snapshot_buffer = NULL;
	core_serialize(true);
	snapshot_size = snapshot_max + SNAPSHOT_OVERHEAD;
	if (snapshot_size % SNAPSHOT_ROUND)
		snapshot_size += (SNAPSHOT_ROUND - (snapshot_size % SNAPSHOT_ROUND));

	return true;
}

RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
	return false;
}

RETRO_API void retro_unload_game(void)
{
	// TODO save floppy replacement?
}

RETRO_API unsigned retro_get_region(void)
{
	// Using the TOS language for this (see tos.h).
	// 0 is US, -1 is unknown, 127+ is "all", and I think all the other region enums are in europe?
	int lang = TOS_DefaultLanguage();
	if (lang <= 0 || lang >= 127) return RETRO_REGION_NTSC;
	return RETRO_REGION_PAL;
}

RETRO_API void* retro_get_memory_data(unsigned id)
{
	if (id == RETRO_MEMORY_SYSTEM_RAM) return STRam;
	return NULL;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
	if (id == RETRO_MEMORY_SYSTEM_RAM) return STRamEnd;
	return 0;
}
