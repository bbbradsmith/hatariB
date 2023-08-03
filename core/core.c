#include "../libretro/libretro.h"
#include "core.h"
#include <string.h>

// large enough for TT high resolution 1280x960 at 32bpp
#define VIDEO_MAX_W   2048
#define VIDEO_MAX_H   1024
#define VIDEO_MAX_PITCH   (VIDEO_MAX_W*4)
// 4 frames of buffer at slowest framerate
#define AUDIO_BUFFER_LEN   (4*2*96000/50)

//
// Libretro
//

static void null_log(enum retro_log_level, const char*, ...) {}

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
// Core implementation in other files
//

// core_input.c
extern void core_input_init(); // call in retro_init
extern void core_input_update(); // call in retro_run, polls Libretro inputs and translates to events for hatari

//
// Available to Hatari
//

// external
int core_pixel_format = 0;

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
	// TODO any response needed?
}

RETRO_API void retro_reset(void)
{
	// TODO reset emulator (reset button?)
}

RETRO_API void retro_run(void)
{
	// poll input, generate event queue for hatari
	core_input_update();
	
	// run one frame
	m68k_go_frame();

	// TODO Libretro video output can't handle variable framerate,
	// so the wrong framerate has to be converted.
	// We could use retro_get_system_av_info to set it at game load time, if known?

	// send video
	if (core_rate_changed)
	{
		struct retro_system_av_info info;
		retro_get_system_av_info(&info);
		environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
		core_rate_changed = false;
		core_video_changed = false;
	}
	else if (core_video_changed)
	{
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
}

RETRO_API size_t retro_serialize_size(void)
{
	// TODO get savestate size
	return 0;
}

RETRO_API bool retro_serialize(void *data, size_t size)
{
	// TODO savesatate save
	return false;
}

RETRO_API bool retro_unserialize(const void *data, size_t size)
{
	// TODO savestate load
	return false;
}

RETRO_API void retro_cheat_reset(void)
{
	// TODO response needed?
}

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	// TODO response needed? does hatari have cheats?
}

RETRO_API bool retro_load_game(const struct retro_game_info *game)
{
	// TODO
	return true;
}

RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
	// TODO
	return false;
}

RETRO_API void retro_unload_game(void)
{
	// TODO
}

RETRO_API unsigned retro_get_region(void)
{
	// TODO
	return 0;
}

RETRO_API void* retro_get_memory_data(unsigned id)
{
	// TODO
	return NULL;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
	// TODO
	return 0;
}
