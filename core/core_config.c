#include "../libretro/libretro.h"
#include "core.h"
#include "../hatari/src/includes/main.h"
#include "../hatari/src/includes/configuration.h"

extern retro_environment_t environ_cb;
extern retro_log_printf_t retro_log;

extern int core_video_aspect_mode;
extern bool core_video_changed;

static CNF_PARAMS defparam; // TODO copy this during setup
static CNF_PARAMS newparam; // TODO copy default to this then modify during apply

static struct retro_core_option_v2_category CORE_OPTION_CAT[] = {
	{
		"video", "Video",
		"Video settings"
	},
	{
		"audio", "Audio",
		"Audio settings"
	},
	{ NULL, NULL, NULL }
};

static struct retro_core_option_v2_definition CORE_OPTION_DEF[] = {
	{
		"lowres2x",
		"Video > Low Resolution Double",
		"Low Resolution Double",
		"Low resolution pixel doubling for 640x400 screen output."
		" Prevents video output size changes for resolution switch.",
		NULL,
		"video",
		{
			{"0","Off"},
			{"1","On"},
			{NULL,NULL}
		}, "1"
	},
	{
		"borders",
		"Video > Screen Borders",
		"Screen Borders",
		"Atari ST monitors had a visible border around the main screen area,"
		" but most software does not display anything in it.",
		NULL,
		"video",
		{
			{"0","Hide"},
			{"1","Show"},
			{NULL,NULL}
		}, "1"
	},
	{
		"statusbar",
		"Video > Status Bar",
		"Status Bar",
		"Display the Hatari status bar at the bottom of the screen,"
		" or floppy drive light at the top right.",
		NULL,
		"video",
		{
			{"0","Off"},
			{"1","On"},
			{"2","Drive Light"},
			{NULL,NULL}
		}, "1"
	},
	{
		"aspect",
		"Video > Pixel Aspect Ratio",
		"Pixel Aspect Ratio",
		"Reports a pixel aspect ratio appropriate for a chosen monitor type."
		" Requires 'Core Provided' Aspect Ratio in Video > Scaling settings.",
		NULL,
		"video",
		{
			{"0","Square Pixels"},
			{"1","Atari Monitor"},
			{"2","NTSC TV"},
			{"3","PAL TV"},
			{NULL,NULL}
		}, "1"
	},
	{
		"lpf",
		"Audio > Lowpass Filter",
		"Lowpass Filter",
		"Reduces high frequency noise from sound output.",
		NULL,
		"audio",
		{
			{"0","None"},
			{"1","STF Lowpass"},
			{"2","Piecewise Selective Lowpass"},
			{"3","Clean Lowpass"},
			{NULL,NULL}
		}, "3"
	},
	{
		"hpf",
		"Audio > Highpass Filter",
		"Highpass Filter",
		"Removes very low frequencies to keep output waveform centred.",
		NULL,
		"audio",
		{
			{"0","None"},
			{"1","IIR Highpass"},
			{NULL,NULL}
		}, "1"
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, {{NULL,NULL}}, NULL }
};

static struct retro_core_options_v2 CORE_OPTIONS_V2 = {
	CORE_OPTION_CAT,
	CORE_OPTION_DEF,
};

//
// from Hatari
//

extern CNF_PARAMS ConfigureParams;
extern bool Change_DoNeedReset(CNF_PARAMS *current, CNF_PARAMS *changed);
extern void Change_CopyChangedParamsToConfiguration(CNF_PARAMS *current, CNF_PARAMS *changed, bool bForceReset);

//
// Internal
//

bool cfg_read_int(const char* key, int* v)
{
	struct retro_variable var = { key, NULL };
	if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE,&var)) return false;
	if (v)
	{
		*v = 0;
		if (var.value) *v = atoi(var.value);
	}
	return true;
}

#define CFG_INT(key) if (cfg_read_int((key),&vi))

void core_config_read_newparam()
{
	int vi;
	newparam = defparam; // start with the defaults
	CFG_INT("lowres2x") newparam.Screen.bLowResolutionDouble = vi;
	CFG_INT("borders") newparam.Screen.bAllowOverscan = vi;
	CFG_INT("statusbar") { newparam.Screen.bShowStatusbar = (vi==1); newparam.Screen.bShowDriveLed = (vi==2); }
	CFG_INT("aspect") { if (core_video_aspect_mode != vi) { core_video_aspect_mode = vi; core_video_changed = true; } }
	CFG_INT("lpf") newparam.Sound.YmLpf = vi;
	CFG_INT("hpf") newparam.Sound.YmHpf = vi;
}

//
// Interface
//

void core_config_set_environment(retro_environment_t cb)
{
	unsigned version = 0;
	if (!cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version)) version = 0;
	if (version >= 2)
	{
		cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &CORE_OPTIONS_V2);
	}
	else if (version >= 1)
	{
		// v1
		// parse into v1? maybe just fall back to v0
		// TODO
	}
	else
	{
		// retro_variable ?
		// parse into values and send individually?
		// TODO
	}
}

void core_config_init(void) // called from hatari after setting defaults
{
	defparam = ConfigureParams; // save copy of defaults
	core_config_read_newparam();
	ConfigureParams = newparam;
}

void core_config_apply(void)
{
	core_config_read_newparam();
	bool reset = Change_DoNeedReset(&ConfigureParams, &newparam);
	Change_CopyChangedParamsToConfiguration(&ConfigureParams, &newparam, reset);
}
