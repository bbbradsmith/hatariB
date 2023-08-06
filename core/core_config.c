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

// system/hatarib/ file scane used to populate arrays in CORE_OPTION_DEF below
#define MAX_OPTION_FILES   64

static struct retro_core_option_v2_category CORE_OPTION_CAT[] = {
	{
		"system", "System",
		"TOS, Monitor, Floppy, other settings."
	},
	{
		"input", "Input",
		"Attached joysticks, mouse, and keyboard setup."
	},
	{
		"video", "Video",
		"On-screen display, borders, etc."
	},
	{
		"audio", "Audio",
		"Sound settings."
	},
	{
		"advanced", "Advanced",
		"Advanced Hatari settings."
	},
	{
		"pad1", "Retropad 1",
		"Input pad 1 customization."
	},
	{
		"pad2", "Retropad 2",
		"Input pad 2 customization."
	},
	{
		"pad3", "Retropad 3",
		"Input pad 3 customization."
	},
	{
		"pad4", "Retropad 4",
		"Input pad 4 customization."
	},
	{ NULL, NULL, NULL }
};

static struct retro_core_option_v2_definition CORE_OPTION_DEF[] = {
	//
	// System
	//
	{
		"tos", "*TOS ROM", NULL,
		"BIOS ROM can use built-in EmuTOS, or choose from:\n"
		" system/tos.img (default)\n"
		" system/hatarib/* (all .img, .rom, .bin in folder)\n",
		NULL, "system",
		{
			{"<tos.img>","system/tos.img"}, // add (missing) to description if it's missing, set etos1024k as default in that case
			{"<etos1024k>","EmuTOS 1024k"},
			{"<etos192uk>","EmuTOS 192uk"},
			{"<etos192us>","EmuTOS 192us"},
			// populated by system/hatarib/ (MAX_OPTION_FILES)
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL}
		}, "<etos1024k>" // replace with <tos.img> if system/tos.img exists
	},
	{
		"monitor", "Monitor", NULL,
		"Causes restart. Monitor type. Colour, monochrome, etc.",
		NULL, "system",
		{
			{"0","Monochrome High-Resolution"},
			{"1","RGB Colour Low/Medium-Resolution"},
			{"2","VGA"},
			{"3","TV"},
			{NULL,NULL},
		}, "1"
	},
	{
		"fast_floppy", "*Fast Floppy", NULL,
		"Artifically accelerate floppy disk access, reducing load times.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"save_floppy", "*Save Floppy Disks", NULL,
		"Changes to floppy disks will save a copy in saves/."
		" If turned off, changes will be lost when the content is closed.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"reset", "*Hard Reset", NULL,
		"Core reset is a warm boot reset, but this option will force a full cold boot instead.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"machine", "*Machine Type", NULL,
		"Atari computer type.",
		NULL, "system",
		{
			{"0","ST"},
			{"1","Mega ST"},
			{"2","STE"},
			{"3","Mega STE"},
			{"4","TT"},
			{"5","Falcon"},
			{NULL,NULL},
		},"0"
	},
	{
		"memory", "*ST Memory Size", NULL,
		"Atari ST memory size.",
		NULL, "system",
		{
			{"256","256 KB"},
			{"512","512 KB"},
			{"1024","1 MB"},
			{"2048","2 MB"},
			{"2560","2.5 MB"},
			{"4096","4 MB"},
			{"8192","8 MB"},
			{"10240","10 MB"},
			{"14336","14 MB"},
			{NULL,NULL},
		}, "1024"
	},
	{
		"cartridge", "*Cartridge ROM", NULL,
		"ROM image for cartridge port, list of files from system/hatarib/.",
		NULL, "system",
		{
			{"<none>","None"},
			// populated by system/hatarib/ (MAX_OPTION_FILES)
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL}
		}, "<none>"
	},
	{
		"hardimg", "*Hard Disk", NULL,
		"Hard drive image, list of files from system/hatarib/.",
		NULL, "system",
		{
			{"<none>","None"},
			// populated by system/hatarib/ (MAX_OPTION_FILES)
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL}
		}, "<none>"
	},
	{
		"hardtype", "*Hard Disk Type", NULL,
		"Boot from hard disk.",
		NULL, "system",
		{
			{"0","ACSI"},
			{"1","SCSI"},
			{"2","IDE (Auto)"},
			{"3","IDE (Byte Swap Off)"},
			{"4","IDE (Byte Swap On)"},
			{NULL,NULL},}, "0"
	},
	{
		"hardboot", "*Hard Disk Boot", NULL,
		"Boot from hard disk.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	//
	// Input
	//
	{ "joy1_port", "Joystick 1", NULL, "Retropad 1 assigned Atari port.", NULL, "input", {{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"}},"1" },
	{ "joy2_port", "Joystick 2", NULL, "Retropad 2 assigned Atari port.", NULL, "input", {{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"}},"0" },
	{ "joy3_port", "Joystick 3", NULL, "Retropad 3 assigned Atari port.", NULL, "input", {{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"}},"2" },
	{ "joy4_port", "Joystick 4", NULL, "Retropad 4 assigned Atari port.", NULL, "input", {{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"}},"3" },
	{
		"mouse_port", "*Mouse", NULL,
		"Mouse connected to Joy 0 port."
		" This can be connected at the same time as a joystick, but their inputs will overlap.",
		NULL, "input",
		{{"0","None"},{"1","Joy 0"},{NULL,NULL},}, "1"
	},
	{
		"host_mouse", "*Host Mouse Enabled", NULL,
		"Allow input from your own mouse device."
		" With this disabled you can still use the retropad mouse inputs.",
		NULL, "input",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"host_keyboard", "*Host Keyboard Enabled", NULL,
		"Allow input from your own keyboard."
		" With this disabled you can still use the onscreen keyboard or retropad mapped keys.",
		NULL, "input",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"autofire", "*Auto-fire Rate", NULL,
		"Frames per button press with auto-fire.",
		NULL, "input",
		{
			{"2","2"},
			{"3","3"},
			{"4","4"},
			{"5","5"},
			{"6","6"},
			{"7","7"},
			{"8","8"},
			{"9","9"},
			{"10","10"},
			{"11","11"},
			{"12","12"},
			{"13","13"},
			{"14","14"},
			{"15","15"},
			{"16","16"},
			{"17","17"},
			{"18","18"},
			{"19","19"},
			{"20","20"},
			{NULL,NULL},
		}, "6"
	},
	{
		"mouse_speed", "*Mouse Stick Speed", NULL,
		"Speed of the mouse when controlled by the analog sticks.",
		NULL, "input",
		{
			{"1","1"},
			{"2","2"},
			{"3","3"},
			{"4","4"},
			{"5","5"},
			{NULL,NULL},
		}, "3"
	},
	{
		"mouse_deadzone", "*Mouse Stick Deadzone", NULL,
		"Dead zone for mouse analog stick control to prevent movement from controller randomness.",
		NULL, "input",
		{
			{"0","0%"},
			{"1","1%"},
			{"2","2%"},
			{"3","3%"},
			{"4","4%"},
			{"5","5%"},
			{"6","6%"},
			{"7","7%"},
			{"8","8%"},
			{"9","9%"},
			{"10","10%"},
			{"11","11%"},
			{"12","12%"},
			{"13","13%"},
			{"14","14%"},
			{"15","15%"},
			{"20","20%"},
			{"25","25%"},
			{"30","30%"},
			{"35","35%"},
			{"40","40%"},
			{"45","45%"},
			{"50","50%"},
			{NULL,NULL},
		}, "10"
	},
	//
	// Video
	//
	{
		"lowres2x", "Low Resolution Double", NULL,
		"Low resolution pixel doubling for 640x400 screen output."
		" Prevents video output size changes for resolution switch.",
		NULL, "video",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"borders", "Screen Borders", NULL,
		"Atari ST monitors had a visible border around the main screen area,"
		" but most software does not display anything in it.",
		NULL, "video",
		{
			{"0","Hide"},
			{"1","Show"},
			{NULL,NULL}
		}, "1"
	},
	{
		"statusbar", "Status Bar", NULL,
		"Display the Hatari status bar at the bottom of the screen,"
		" or floppy drive light at the top right.",
		NULL, "video",
		{
			{"0","Off"},
			{"1","On"},
			{"2","Drive Light"},
			{NULL,NULL}
		}, "1"
	},
	{
		"aspect", "Pixel Aspect Ratio", NULL,
		"Reports a pixel aspect ratio appropriate for a chosen monitor type."
		" Requires 'Core Provided' Aspect Ratio in Video > Scaling settings.",
		NULL, "video",
		{
			{"0","Square Pixels"},
			{"1","Atari Monitor"},
			{"2","NTSC TV"},
			{"3","PAL TV"},
			{NULL,NULL}
		}, "1"
	},
	//
	// Audio
	//
	{
		"samplerate", "*Samplerate", NULL,
		"Audio samplerate.",
		NULL, "audio",
		{
			{"11025","11025 Hz"},
			{"16000","16000 Hz"},
			{"22050","22050 Hz"},
			{"32000","32000 Hz"},
			{"44100","44100 Hz"},
			{"48000","48000 Hz"},
			{"50066","55066 Hz"},
			{NULL,NULL}
		},"48000"
	},
	{
		"ymmix", "*YM Voices Mixing", NULL,
		"Sound chip volume curves.",
		NULL, "audio",
		{
			{"1","Linear"},
			{"2","ST Table"},
			{"3","Math Model"},
			{NULL,NULL}
		},"2"
	},
	{
		"lpf", "Lowpass Filter", NULL,
		"Reduces high frequency noise from sound output.",
		NULL, "audio",
		{
			{"0","None"},
			{"1","STF Lowpass"},
			{"2","Piecewise Selective Lowpass"},
			{"3","Clean Lowpass"},
			{NULL,NULL}
		}, "3"
	},
	{
		"hpf", "Highpass Filter", NULL,
		"Removes very low frequencies to keep output waveform centred.",
		NULL, "audio",
		{
			{"0","None"},
			{"1","IIR Highpass"},
			{NULL,NULL}
		}, "1"
	},
	//
	// Advanced
	//
	{
		"driveb","*Drive B Enable", NULL,
		"Turn off to disconnect drive B.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"drivesingle","*Single-Sided Drives", NULL,
		"Single-Sided floppy drives instead of Double-Sided.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"readonly_floppy","*Write Protect Floppy Disks", NULL,
		"Write-protect all floppy disks in emulation."
		" The emulated operating system will know that all writes are failing.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"blitter_st","*Blitter in ST Mode", NULL,
		"Normally the blitter requires a Mega ST.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"patchtos","*Patch TOS for Fast Boot", NULL,
		"Boot slightly faster for some known TOS ROMs.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"prefetch", "*Prefetch Emulation", NULL,
		"Uses more CPU power, more accurate.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"cycle_exact", "*Cycle-exact with cache emulation", NULL,
		"Uses more CPU power, more accurate.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"mmu", "*MMU Emulation", NULL,
		"For TT or Falcon. Uses more CPU power.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"wakestate", "*Video Timing", NULL,
		"Specify startup timing for video output.",
		NULL, "advanced",
		{
			{"0","Random"},
			{"1","Wakestate 1"},
			{"2","Wakestate 2"},
			{"3","Wakestate 3"},
			{"4","Wakestate 4"},
			{NULL,NULL}
		}, "3",
	},
	{
		"timerd","*Patch Timer-D", NULL,
		"Who knows what this does?",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	//
	// Pads
	//
#define OPTION_PAD_STICK() \
	{ \
		{"0","None"}, \
		{"1","Joystick"}, \
		{"2","Mouse"}, \
		{"3","Cursor Keys"}, \
		{NULL,NULL} \
	}
#define OPTION_PAD_BUTTON() \
	{ \
		{"0","None"}, \
		{"1","Fire"}, \
		{"2","Auto-Fire"}, \
		{"3","Mouse Left"}, \
		{"4","Mouse Right"}, \
		{"5","On-Screen Keyboard"}, \
		{"6","On-Screen Keyboard One-Shot"}, \
		{"7","Select Floppy Drive"}, \
		{"8","Help Screen"}, \
		{"9","STE Button A"}, \
		{"10","STE Button B"}, \
		{"11","STE Button C"}, \
		{"12","STE Button Option"}, \
		{"13","STE Button Pause"}, \
		{"14","Key Space"}, \
		{"15","Key Enter"}, \
		{"16","Key Up"}, \
		{"17","Key Down"}, \
		{"18","Key Left"}, \
		{"19","Key Right"}, \
		{"20","Key 1"}, \
		{"21","Key 2"}, \
		{"22","Key 3"}, \
		{"23","Key 4"}, \
		{"24","Key 5"}, \
		{"25","Key 6"}, \
		{"26","Key 7"}, \
		{"27","Key 8"}, \
		{"28","Key 9"}, \
		{"29","Key 0"}, \
		/* TODO lots more */ \
		{NULL,NULL} \
	}
#define OPTION_OSKEY_BUTTON() \
	{ \
		{"0","A"}, \
		{"1","B"}, \
		{"2","X"}, \
		{"3","Y"}, \
		{"4","Select"}, \
		{"5","Start"}, \
		{"6","L1"}, \
		{"7","R1"}, \
		{"8","L2"}, \
		{"9","R2"}, \
		{"10","L3"}, \
		{"11","R3"}, \
		{NULL,NULL} \
	}
#define OPTION_PAD(padnum) \
	{ "pad" padnum "_dpad", "Pad " padnum " D-Pad", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_STICK(), "1" }, /* joystick */ \
	{ "pad" padnum "_a", "Pad " padnum " A", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "2" }, /* auto-fire */ \
	{ "pad" padnum "_b", "Pad " padnum " B", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "1" }, /* fire */ \
	{ "pad" padnum "_x", "Pad " padnum " X", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "4" }, /* mouse right */ \
	{ "pad" padnum "_y", "Pad " padnum " Y", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "3" }, /* mouse left */ \
	{ "pad" padnum "_select", "Pad " padnum " Select", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "7" }, /* select floppy drive */ \
	{ "pad" padnum "_start", "Pad " padnum " Start", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "8" }, /* help screen */ \
	{ "pad" padnum "_l1", "Pad " padnum " L1", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "5" }, /* on-screen keyboard */ \
	{ "pad" padnum "_r1", "Pad " padnum " R1", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "6" }, /* on-screen keyboard one-shot */ \
	{ "pad" padnum "_l2", "Pad " padnum " L2", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "0" }, /* none */ \
	{ "pad" padnum "_r2", "Pad " padnum " R2", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "0" }, /* none */ \
	{ "pad" padnum "_l3", "Pad " padnum " L3", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "14" }, /* key space */ \
	{ "pad" padnum "_r3", "Pad " padnum " R3", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "15" }, /* key enter */ \
	{ "pad" padnum "_lstick", "Pad " padnum " Left Analog Stick", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_STICK(), "1" }, /* joystick */ \
	{ "pad" padnum "_rstick", "Pad " padnum " Right Analog Stick", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_STICK(), "2" }, /* mouse */ \
	{ "pad" padnum "_oskey_confirm", "Pad " padnum " On-Screen Key Confirm", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "1" }, /* b */ \
	{ "pad" padnum "_oskey_cancel", "Pad " padnum " On-Screen Key Cancel", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "0" }, /* a */ \
	{ "pad" padnum "_oskey_shift", "Pad " padnum " On-Screen Key Shift-Lock", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "3" }, /* y */ \
	{ "pad" padnum "_oskey_pos", "Pad " padnum " On-Screen Key Position", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "2" }, /* x */ \
	{ "pad" padnum "_oskey_move", "Pad " padnum " On-Screen Key Cursor", NULL, NULL, NULL, "pad" padnum, \
		{{"0","D-Pad"},{"1","Left Analog Stick"},{"2","Right Analog Stick"},{NULL,NULL}},"0" }, /* d-pad */ \
	/* end of OPTION_PAD define */
	OPTION_PAD("1")
	OPTION_PAD("2")
	OPTION_PAD("3")
	OPTION_PAD("4")
	//
	// End
	//
	{ NULL, NULL, NULL, NULL, NULL, NULL, {{NULL,NULL}}, NULL }
};

static const struct retro_core_options_v2 CORE_OPTIONS_V2 = {
	CORE_OPTION_CAT,
	CORE_OPTION_DEF,
};

static const struct retro_input_descriptor INPUT_DESCRIPTORS[] = {
#define PAD_DESCRIPTOR(p) \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A / Auto-Fire" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B / Fire" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X / Mouse Right" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y / Mouse Left" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select / Select Drive" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start / Help" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L1 / On-Screen Keyboard" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R1 / On-Screen Keyboard One-Shot" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3 / Key Space" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3 / Key Enter" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up / Joystick" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down / Joystick" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left / Joystick" }, \
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right / Joystick" }, \
	{ p, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Left Analog X Stick / Joystick" }, \
	{ p, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Left Analog Y Stick / Joystick" }, \
	{ p, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Right Analog X Stick / Mouse" }, \
	{ p, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Right Analog Y Stick / Mouse" }, \
	/* end of PAD_DESCRIPTOR define */
	PAD_DESCRIPTOR(0)
	PAD_DESCRIPTOR(1)
	PAD_DESCRIPTOR(2)
	PAD_DESCRIPTOR(3)
	{0}
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

static struct retro_variable cfg_var = { NULL, NULL };

bool cfg_read_int(const char* key, int* v)
{
	cfg_var.key = key;
	cfg_var.value = NULL;
	if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE,&cfg_var)) return false;
	if (v)
	{
		*v = 0;
		if (cfg_var.value) *v = atoi(cfg_var.value);
	}
	return true;
}

bool cfg_read_str(const char* key, const char** s)
{
	cfg_var.key = key;
	cfg_var.value = NULL;
	if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE,&cfg_var)) return false;
	if (!cfg_var.value) return false;
	if (s) *s = cfg_var.value;
	return true;
}

bool cfg_read_int_pad(int pad, const char* key, int* v)
{
	static char padkey[64];
	snprintf(padkey,sizeof(padkey),"pad%d_%s",pad,key);
	padkey[sizeof(padkey)-1] = 0;
	return cfg_read_int(padkey,v);
}

#define CFG_INT(key) if (cfg_read_int((key),&vi))
#define CFG_STR(key) if (cfg_read_str((key),&vs))
#define CFG_INT_PAD(pad,key) if (cfg_read_int_pad((pad),(key),&vi))

void core_config_read_newparam()
{
	int vi;
	const char* vs;
	newparam = defparam; // start with the defaults
	CFG_STR("tos") {} // TODO
	CFG_INT("monitor") newparam.Screen.nMonitorType = vi;
	CFG_INT("fast_floppy") {} // TODO
	CFG_INT("save_floppy") {} // TODO
	CFG_INT("reset") {} // TODO
	CFG_INT("machine") {
		// automatic setup based on OPT_MACHINE in options.c
		static const int CPULEVEL[] = { 0, 0, 0, 0, 3, 3 };
		static const int CPUFREQ[]  = { 8, 8, 8, 8,32,16 };
		newparam.System.nMachineType = vi;
		newparam.System.nCpuLevel = CPULEVEL[vi];
		newparam.System.nCpuFreq = CPUFREQ[vi];
		if (vi == MACHINE_TT)
		{
			newparam.System.bCompatibleFPU = true;
			newparam.System.n_FPUType = FPU_68882;
		}
		else if (vi == MACHINE_FALCON)
		{
			newparam.System.nDSPType = DSP_TYPE_EMU;
		}
	}
	CFG_INT("memory") {} // TODO
	CFG_STR("cartridge") {} // TODO
	CFG_STR("hardimg") {} // TODO
	CFG_INT("hardboot") {} // TODO
	CFG_INT("hardtype") {} // TODO
	CFG_INT("joy1_port") {} // TODO
	CFG_INT("joy2_port") {} // TODO
	CFG_INT("joy3_port") {} // TODO
	CFG_INT("joy4_port") {} // TODO
	CFG_INT("mouse_port") {} // TODO
	CFG_INT("host_mouse") {} // TODO
	CFG_INT("host_keyboard") {} // TODO
	CFG_INT("autofire") {} // TODO
	CFG_INT("mouse_speed") {} // TODO
	CFG_INT("mouse_deadzone") {} // TODO
	CFG_INT("lowres2x") newparam.Screen.bLowResolutionDouble = vi;
	CFG_INT("borders") newparam.Screen.bAllowOverscan = vi;
	CFG_INT("statusbar") { newparam.Screen.bShowStatusbar = (vi==1); newparam.Screen.bShowDriveLed = (vi==2); }
	CFG_INT("aspect") { if (core_video_aspect_mode != vi) { core_video_aspect_mode = vi; core_video_changed = true; } }
	CFG_INT("samplerate") {} // TODO
	CFG_INT("ymmix") {} // TODO
	CFG_INT("lpf") newparam.Sound.YmLpf = vi;
	CFG_INT("hpf") newparam.Sound.YmHpf = vi;
	CFG_INT("driveb") {} // TODO
	CFG_INT("drivesingle") {} // TODO
	CFG_INT("readonly_floppy") {} // TODO
	CFG_INT("blitterst") {} // TODO
	CFG_INT("readonly_floppy") {} // TODO
	CFG_INT("blitter_st") {} // TODO
	CFG_INT("patchtos") {} // TODO
	CFG_INT("prefetch") {} // TODO
	CFG_INT("cycle_exact") {} // TODO
	CFG_INT("mmu") {} // TODO
	CFG_INT("wakestate") {} // TODO
	CFG_INT("timerd") {} // TODO
	for (int i=0; i<4; ++i)
	{
		CFG_INT_PAD(i,"dpad") {} // TODO
		CFG_INT_PAD(i,"a") {} // TODO
		CFG_INT_PAD(i,"b") {} // TODO
		CFG_INT_PAD(i,"x") {} // TODO
		CFG_INT_PAD(i,"y") {} // TODO
		CFG_INT_PAD(i,"select") {} // TODO
		CFG_INT_PAD(i,"start") {} // TODO
		CFG_INT_PAD(i,"l1") {} // TODO
		CFG_INT_PAD(i,"r1") {} // TODO
		CFG_INT_PAD(i,"l2") {} // TODO
		CFG_INT_PAD(i,"r2") {} // TODO
		CFG_INT_PAD(i,"l3") {} // TODO
		CFG_INT_PAD(i,"r3") {} // TODO
		CFG_INT_PAD(i,"lstick") {} // TODO
		CFG_INT_PAD(i,"rstick") {} // TODO
		CFG_INT_PAD(i,"oskey_confirm") {} // TODO
		CFG_INT_PAD(i,"oskey_cancel") {} // TODO
		CFG_INT_PAD(i,"oskey_shift") {} // TODO
		CFG_INT_PAD(i,"oskey_pos") {} // TODO
		CFG_INT_PAD(i,"oskey_move") {} // TODO
	}
}

//
// Interface
//

static struct retro_core_option_v2_definition* get_core_option_def(const char* key)
{
	for (struct retro_core_option_v2_definition* d = CORE_OPTION_DEF; d->key != NULL; ++d)
	{
		if (!strcmp(d->key,key)) return d;
	}
	return NULL;
}

void core_config_set_environment(retro_environment_t cb)
{
	unsigned version = 0;
	struct retro_core_option_v2_definition* def;

	// TODO fill in TOS / Cartridge / Hard Disk lists from system/hatarib scan
	if ((def = get_core_option_def("tos")))
	{
		// populate up to MAX_OPTION_FILES starting from 4
		def->values[4].value = "pop.tos";
		def->values[4].label = "pop.tos (test)";
		// if tos.img is found we should do this:
		def->default_value = "<tos.img>";
		// if tos.img is not found we should do this:
		def->values[0].label = "system/tos.img (missing)";
	}
	if ((def = get_core_option_def("cartridge")))
	{
		def->values[1].value = "pop.img";
		def->values[1].label = "pop.img (test)";
	}
	if ((def = get_core_option_def("hardimg")))
	{
		def->values[1].value = "pop.img";
		def->values[1].label = "pop.img (test)";
	}

	if (!cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version)) version = 0;
	if (version >= 2)
	{
		cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, (void*)&CORE_OPTIONS_V2);
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

	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)INPUT_DESCRIPTORS);
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
