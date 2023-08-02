#include "../libretro/libretro.h"
#include "core.h"

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
// Available to Hatari
//

void core_debug_log(const char* msg)
{
	retro_log(RETRO_LOG_DEBUG,msg);
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

	main_init(1,(char**)argv); // TODO how are paths affected?
}

RETRO_API void retro_deinit(void)
{
	retro_log(RETRO_LOG_INFO,"retro_deinit()\n");

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
	// TODO
	info->geometry.base_width = 640; // TODO 320, overscan
	info->geometry.base_height = 400; // TODO 200, overscan
	info->geometry.max_width = 640; // TODO overscan
	info->geometry.max_height = 400; // TODO overscan
	info->geometry.aspect_ratio = 0; // TODO PAR (with NTSC, PAL options, hires is 1:1?)
	info->timing.fps = 60; // TODO NTSC/PAL/hires
	info->timing.sample_rate = 48000; // TODO
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
	// TODO any response needed?
}

RETRO_API void retro_reset(void)
{
	// TODO reset emulator (reset button?)
}

void test_vid(uint16_t* vid, int x, int y, int o)
{
	unsigned char r, g, b;
	int i = ((y*640)+x);
	x += o; // scroll horizontally
	r = (unsigned char)(x << 4); // red ramp 16 pixels horizontal
	g = (unsigned char)(y << 4); // green ramp 16 pixels vertical
	b = (unsigned char)(x & 0xF0); // blue ramp across 256 pixels horizontal
	vid[i] = 0x8000
		| ((r & 0xF8) << 7)
		| ((g & 0xF8) << 2)
		| ((b & 0xF8) >> 3);
}

RETRO_API void retro_run(void)
{
	// TODO run one frame

	// poll input
	input_poll_cb();
	
	// test video
	static uint16_t vid[640*400*4];
	static int vidanim = 0; ++vidanim;
	for (int y=0;y<400;++y)
		for (int x=0;x<640;++x)
			test_vid(vid,x,y,vidanim);
	video_cb(vid,640,400,640*2);

	// saw wave audio
	const int SPF = 48000/60;
	const int WAVELEN = 48000 / 200;
	const int WAVEAMP = 1000;
	int16_t aud[SPF*2];
	static int wavepos = 0;
	for (int i=0;i<SPF;++i) { aud[(i*2)+0] = aud[(i*2)+1] = ((wavepos*WAVEAMP)/WAVELEN); ++wavepos; if(wavepos>WAVELEN) wavepos=0; }
	audio_batch_cb(aud,SPF);
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
