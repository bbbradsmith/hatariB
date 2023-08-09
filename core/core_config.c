#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include "../hatari/src/includes/main.h"
#include "../hatari/src/includes/configuration.h"

static CNF_PARAMS defparam; // TODO copy this during setup
static CNF_PARAMS newparam; // TODO copy default to this then modify during apply

// system/hatarib/ file scane used to populate arrays in CORE_OPTION_DEF below
#define MAX_OPTION_FILES   100

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
		"hatarib_tos", "TOS ROM", NULL,
		"Causes restart!! BIOS ROM can use built-in EmuTOS, or choose from:\n"
		" system/tos.img (default)\n"
		" system/hatarib/* (all .img, .rom, .bin in folder)\n",
		NULL, "system",
		{
			{"<tos.img>","system/tos.img"}, // add (missing) to description if it's missing, set etos1024k as default in that case
			{"<etos1024k>","EmuTOS 1024k"},
			{"<etos192uk>","EmuTOS 192uk"},
			{"<etos192us>","EmuTOS 192us"},
			// populated by system/hatarib/ (MAX_OPTION_FILES)
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL}
		}, "<etos1024k>" // replace with <tos.img> if system/tos.img exists
	},
	{
		"hatarib_monitor", "Monitor", NULL,
		"Causes restart!! Monitor type. Colour, monochrome, etc.",
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
		"hatarib_fast_floppy", "Fast Floppy", NULL,
		"Artifically accelerate floppy disk access, reducing load times.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"hatarib_save_floppy", "*Save Floppy Disks", NULL,
		"Changes to floppy disks will save a copy in saves/."
		" If turned off, changes will be lost when the content is closed.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"hatarib_hard_reset", "Hard Reset", NULL,
		"Core Restart is a warm boot (reset button), but this option will use full cold boot instead (power off, on).",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"hatarib_machine", "Machine Type", NULL,
		"Causes restart!! Atari computer type.",
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
		"hatarib_memory", "ST Memory Size", NULL,
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
		"hatarib_cartridge", "Cartridge ROM", NULL,
		"ROM image for cartridge port, list of files from system/hatarib/.",
		NULL, "system",
		{
			{"<none>","None"},
			// populated by system/hatarib/ (MAX_OPTION_FILES)
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL}
		}, "<none>"
	},
	{
		"hatarib_hardimg", "Hard Disk", NULL,
		"Hard drive image, list of files and directories from system/hatarib/.",
		NULL, "system",
		{
			{"<none>","None"},
			// populated by system/hatarib/ (MAX_OPTION_FILES)
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},{NULL,NULL},
			{NULL,NULL}
		}, "<none>"
	},
	{
		"hatarib_hardtype", "*Hard Disk Type", NULL,
		"GemDOS type will simulate a hard disk from a folder in system/hatarib/."
		" The other types must use an image file.",
		NULL, "system",
		{
			{"0","GemDOS"},
			{"1","ACSI"},
			{"2","SCSI"},
			{"3","IDE (Auto)"},
			{"4","IDE (Byte Swap Off)"},
			{"5","IDE (Byte Swap On)"},
			{NULL,NULL},}, "0"
	},
	{
		"hatarib_hardboot", "Hard Disk Boot", NULL,
		"Boot from hard disk.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"hatarib_writeprotect", "GemDOS Hard Disk Write Protect", NULL,
		"Write protect the GemDOS hard disk folder.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{"2","Auto"},{NULL,NULL},}, "0"
	},
	//
	// Input
	//
	{ "hatarib_joy1_port", "Joystick 1", NULL, "Retropad 1 assigned Atari port.", NULL, "input", {{"6","None"},{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"}},"1" },
	{ "hatarib_joy2_port", "Joystick 2", NULL, "Retropad 2 assigned Atari port.", NULL, "input", {{"6","None"},{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"}},"0" },
	{ "hatarib_joy3_port", "Joystick 3", NULL, "Retropad 3 assigned Atari port.", NULL, "input", {{"6","None"},{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"}},"2" },
	{ "hatarib_joy4_port", "Joystick 4", NULL, "Retropad 4 assigned Atari port.", NULL, "input", {{"6","None"},{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"}},"3" },
	{
		"hatarib_mouse_port", "Mouse", NULL,
		"Mouse connected to Joy 0 port."
		" This can be connected at the same time as a joystick, but their inputs will overlap.",
		NULL, "input",
		{{"0","None"},{"1","Joy 0"},{NULL,NULL},}, "1"
	},
	{
		"hatarib_host_mouse", "Host Mouse Enabled", NULL,
		"Allow input from your own mouse device."
		" With this disabled you can still use the retropad mouse inputs.",
		NULL, "input",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"hatarib_host_keyboard", "Host Keyboard Enabled", NULL,
		"Allow input from your own keyboard."
		" With this disabled you can still use the onscreen keyboard or retropad mapped keys.",
		NULL, "input",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"hatarib_autofire", "Auto-Fire Rate", NULL,
		"Frames per button press with auto-fire. (Lower number is faster.)",
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
		"hatarib_stick_threshold", "Analog Stick Threshold", NULL,
		"How far to tilt in a direction to activate the joystick direction,"
		" if mapped to an analog stick.",
		NULL, "input",
		{
			{"5", "5%" },
			{"10","10%"},
			{"20","20%"},
			{"30","30%"},
			{"40","40%"},
			{"50","50%"},
			{"60","60%"},
			{"70","70%"},
			{"80","80%"},
			{"90","90%"},
			{"95","95%"},
			{NULL,NULL},
		}, "30"
	},
	{
		"hatarib_mouse_speed", "Mouse Stick Speed", NULL,
		"Speed of the mouse when controlled by the analog sticks.",
		NULL, "input",
		{
			{"1","1"},
			{"2","2"},
			{"3","3"},
			{"4","4"},
			{"5","5"},
			{"6","6"},
			{"7","7"},
			{"8","8"},
			{"9","9"},
			{"10","10"},
			{NULL,NULL},
		}, "4"
	},
	{
		"hatarib_mouse_deadzone", "Mouse Stick Deadzone", NULL,
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
		}, "5"
	},
	//
	// Video
	//
	{
		"hatarib_lowres2x", "Low Resolution Double", NULL,
		"Low resolution pixel doubling for 640x400 screen output."
		" Prevents video output size changes for resolution switch.",
		NULL, "video",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"hatarib_borders", "Screen Borders", NULL,
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
		"hatarib_statusbar", "Status Bar", NULL,
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
		"hatarib_aspect", "Pixel Aspect Ratio", NULL,
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
		"hatarib_samplerate", "Samplerate", NULL,
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
		"hatarib_ymmix", "YM Voices Mixing", NULL,
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
		"hatarib_lpf", "Lowpass Filter", NULL,
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
		"hatarib_hpf", "Highpass Filter", NULL,
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
		"hatarib_driveb","Drive B Enable", NULL,
		"Turn off to disconnect drive B.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"hatarib_drivesingle","Single-Sided Drives", NULL,
		"Single-Sided floppy drives instead of Double-Sided.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"hatarib_readonly_floppy","Write Protect Floppy Disks", NULL,
		"Write-protect all floppy disks in emulation."
		" The emulated operating system will know that all writes are failing.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"hatarib_patchtos","Patch TOS for Fast Boot", NULL,
		"Boot slightly faster for some known TOS ROMs.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"hatarib_blitter_st","Blitter in ST Mode", NULL,
		"Causes restart!! Normally the blitter requires a Mega ST.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"hatarib_wakestate", "Video Timing", NULL,
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
		"hatarib_prefetch", "CPU Prefetch Emulation", NULL,
		"Causes restart!! Uses more CPU power, more accurate.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "1"
	},
	{
		"hatarib_cycle_exact", "Cycle-exact Cache Emulation", NULL,
		"Causes restart!! Uses more CPU power, more accurate.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	{
		"hatarib_mmu", "MMU Emulation", NULL,
		"Causes restart!! For TT or Falcon. Uses more CPU power.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL},}, "0"
	},
	//
	// Pads
	//
	// the option indices must match their implementation in core_input.c
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
		{"14","Soft Reset"}, \
		{"15","Hard Reset"}, \
		{"16","Key Space"}, \
		{"17","Key Return"}, \
		{"18","Key Up"}, \
		{"19","Key Down"}, \
		{"20","Key Left"}, \
		{"21","Key Right"}, \
		{"22","Key 1"}, \
		{"23","Key 2"}, \
		{"24","Key 3"}, \
		{"25","Key 4"}, \
		{"26","Key 5"}, \
		{"27","Key 6"}, \
		{"28","Key 7"}, \
		{"29","Key 8"}, \
		{"30","Key 9"}, \
		{"31","Key 0"}, \
		/* TODO lots more */ \
		{NULL,NULL} \
	}
#define OPTION_OSKEY_BUTTON() \
	{ \
		{"0","None"}, \
		{"1","A"}, \
		{"2","B"}, \
		{"3","X"}, \
		{"4","Y"}, \
		{"5","Select"}, \
		{"6","Start"}, \
		{"7","L1"}, \
		{"8","R1"}, \
		{"9","L2"}, \
		{"10","R2"}, \
		{"11","L3"}, \
		{"12","R3"}, \
		{NULL,NULL} \
	}
#define OPTION_PAD(padnum) \
	{ "hatarib_pad" padnum "_dpad", "Pad " padnum " D-Pad", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_STICK(), "1" }, /* joystick */ \
	{ "hatarib_pad" padnum "_a", "Pad " padnum " A", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "2" }, /* auto-fire */ \
	{ "hatarib_pad" padnum "_b", "Pad " padnum " B", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "1" }, /* fire */ \
	{ "hatarib_pad" padnum "_x", "Pad " padnum " X", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "4" }, /* mouse right */ \
	{ "hatarib_pad" padnum "_y", "Pad " padnum " Y", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "3" }, /* mouse left */ \
	{ "hatarib_pad" padnum "_select", "Pad " padnum " Select", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "7" }, /* select floppy drive */ \
	{ "hatarib_pad" padnum "_start", "Pad " padnum " Start", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "8" }, /* help screen */ \
	{ "hatarib_pad" padnum "_l1", "Pad " padnum " L1", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "5" }, /* on-screen keyboard */ \
	{ "hatarib_pad" padnum "_r1", "Pad " padnum " R1", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "6" }, /* on-screen keyboard one-shot */ \
	{ "hatarib_pad" padnum "_l2", "Pad " padnum " L2", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "0" }, /* none */ \
	{ "hatarib_pad" padnum "_r2", "Pad " padnum " R2", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "0" }, /* none */ \
	{ "hatarib_pad" padnum "_l3", "Pad " padnum " L3", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "16" }, /* key space */ \
	{ "hatarib_pad" padnum "_r3", "Pad " padnum " R3", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "17" }, /* key return */ \
	{ "hatarib_pad" padnum "_lstick", "Pad " padnum " Left Analog Stick", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_STICK(), "1" }, /* joystick */ \
	{ "hatarib_pad" padnum "_rstick", "Pad " padnum " Right Analog Stick", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_STICK(), "2" }, /* mouse */ \
	{ "hatarib_pad" padnum "_oskey_confirm", "Pad " padnum " On-Screen Key Confirm", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "2" }, /* b */ \
	{ "hatarib_pad" padnum "_oskey_cancel", "Pad " padnum " On-Screen Key Cancel", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "1" }, /* a */ \
	{ "hatarib_pad" padnum "_oskey_shift", "Pad " padnum " On-Screen Key Shift-Lock", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "4" }, /* y */ \
	{ "hatarib_pad" padnum "_oskey_pos", "Pad " padnum " On-Screen Key Position", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "3" }, /* x */ \
	{ "hatarib_pad" padnum "_oskey_move", "Pad " padnum " On-Screen Key Cursor", NULL, NULL, NULL, "pad" padnum, \
		{ \
			{"0","None"}, \
			{"1","D-Pad"}, \
			{"2","Left Analog Stick"}, \
			{"3","Right Analog Stick"}, \
			{NULL,NULL} \
		},"1" /* d-pad */ \
	}, \
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

#define CORE_OPTIONS_COUNT   CORE_ARRAY_SIZE(CORE_OPTION_DEF)

static struct retro_core_option_definition CORE_OPTIONS_V1[CORE_OPTIONS_COUNT] = {
	{ NULL, NULL, NULL, {{NULL,NULL}}, NULL }
};

static struct retro_variable CORE_OPTIONS_V0[CORE_OPTIONS_COUNT] = {
	{NULL,NULL}
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
	{ p, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3 / Key Return" }, \
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

// extension will be lowercased
// compare against exts list of null terminated strings, ended with a double null
static bool has_extension(const char* fn, const char* exts)
{
	char lowercase_ext[16];
	size_t e = strlen(fn);
	//retro_log(RETRO_LOG_DEBUG,"has_extension('%s',%p)\n",fn,exts);
	for(;e>0;--e)
	{
		if(fn[e] == '.') break;
	}
	if (fn[e] == '.') ++e;
	strcpy_trunc(lowercase_ext,fn+e,sizeof(lowercase_ext));
	for (char* c = lowercase_ext; *c!=0; ++c)
	{
		if (*c >= 'A' && *c <= 'Z') *c += ('a' - 'A');
	}
	//retro_log(RETRO_LOG_DEBUG,"lowercase_ext: '%s'\n",lowercase_ext);
	while (exts[0] != 0)
	{
		//retro_log(RETRO_LOG_DEBUG,"exts: '%s' (%p)\n",exts,exts);
		if (!strcmp(lowercase_ext,exts)) return true;
		exts = exts + strlen(exts) + 1;
	}
	return false;
}

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
	snprintf(padkey,sizeof(padkey),"hatarib_pad%d_%s",pad+1,key);
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
	CFG_STR("hatarib_tos")
	{
		static const char* TOS_LIST[] = {
			"<tos.img>",
			"<etos1024k>",
			"<etos192uk>",
			"<etos192us>",
		};
		#define TOS_LIST_COUNT   CORE_ARRAY_SIZE(TOS_LIST)
		// TOS files will be found in system/hatarib/ (store system/ relative path)
		strcpy_trunc(newparam.Rom.szTosImageFileName,vs,sizeof(newparam.Rom.szTosImageFileName));
		newparam.Rom.nBuiltinTos = 0;
		// if one of the special cases in TOS_LIST, uses system/tos.img or the builtins
		for (int i=0; i<TOS_LIST_COUNT; ++i)
		{
			if (!strcmp(vs,TOS_LIST[i]))
			{
				if (i == 0) // <tos.img> is system/tos.img
					strcpy_trunc(newparam.Rom.szTosImageFileName,"tos.img",sizeof(newparam.Rom.szTosImageFileName));
				newparam.Rom.nBuiltinTos = i;
				break;
			}
		}
	}
	CFG_INT("hatarib_monitor") newparam.Screen.nMonitorType = vi;
	CFG_INT("hatarib_fast_floppy") newparam.DiskImage.FastFloppy = vi;
	CFG_INT("hatarib_save_floppy") core_disk_save = vi;
	CFG_INT("hatarib_hard_reset") core_option_hard_reset = vi;
	CFG_INT("hatarib_machine")
	{
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
	CFG_INT("hatarib_memory") newparam.Memory.STRamSize_KB = vi;
	CFG_STR("hatarib_cartridge")
	{
		if (strcmp(vs,"<none>"))
			strcpy_trunc(newparam.Rom.szCartridgeImageFileName,vs,sizeof(newparam.Rom.szCartridgeImageFileName));
	}
	//CFG_STR("hatarib_hardimg") // handle within hardtype
	CFG_INT("hatarib_hardtype")
	{
		int ht = vi; // store this before using CFG_STR
		const char* image = "<none>";
		CFG_STR("hatarib_hardimg") image = vs;
		if ((image[0] != 0) && strcmp(image,"<none>")) // don't configure unless we have an image to use
		{
			switch(ht)
			{
			default:
			case 0:
				newparam.HardDisk.bUseHardDiskDirectories = true;
				strcpy_trunc(newparam.HardDisk.szHardDiskDirectories[0],image,sizeof(newparam.HardDisk.szHardDiskDirectories[0]));
				break;
			case 1:
				newparam.Acsi[0].bUseDevice = true;
				strcpy_trunc(newparam.Acsi[0].sDeviceFile,image,sizeof(newparam.Acsi[0].sDeviceFile));
				break;
			case 2:
				newparam.Scsi[0].bUseDevice = true;
				strcpy_trunc(newparam.Scsi[0].sDeviceFile,image,sizeof(newparam.Scsi[0].sDeviceFile));
				break;
			case 3:
			case 4:
			case 5:
				newparam.Ide[0].bUseDevice = true;
				strcpy_trunc(newparam.Ide[0].sDeviceFile,image,sizeof(newparam.Scsi[0].sDeviceFile));
				{
					static const BYTESWAPPING BSMAP[3] = { BYTESWAP_AUTO, BYTESWAP_OFF, BYTESWAP_ON };
					newparam.Ide[0].nByteSwap = BSMAP[ht-2];
				}
				break;
			}
		}
	}
	CFG_INT("hatarib_hardboot") newparam.HardDisk.bBootFromHardDisk = vi;
	CFG_INT("hatarib_writeprotect") newparam.HardDisk.nWriteProtection = vi;
	CFG_INT("hatarib_joy1_port") core_joy_port_map[0] = vi;
	CFG_INT("hatarib_joy2_port") core_joy_port_map[1] = vi;
	CFG_INT("hatarib_joy3_port") core_joy_port_map[2] = vi;
	CFG_INT("hatarib_joy4_port") core_joy_port_map[3] = vi;
	CFG_INT("hatarib_mouse_port") core_mouse_port = vi;
	CFG_INT("hatarib_host_mouse") core_host_mouse = vi;
	CFG_INT("hatarib_host_keyboard") core_host_keyboard = vi;
	CFG_INT("hatarib_autofire") core_autofire = vi;
	CFG_INT("hatarib_stick_threshold") core_stick_threshold = vi;
	CFG_INT("hatarib_mouse_speed") core_mouse_speed = vi;
	CFG_INT("hatarib_mouse_deadzone") core_mouse_dead = vi;
	CFG_INT("hatarib_lowres2x") newparam.Screen.bLowResolutionDouble = vi;
	CFG_INT("hatarib_borders") newparam.Screen.bAllowOverscan = vi;
	CFG_INT("hatarib_statusbar") { newparam.Screen.bShowStatusbar = (vi==1); newparam.Screen.bShowDriveLed = (vi==2); }
	CFG_INT("hatarib_aspect") { if (core_video_aspect_mode != vi) { core_video_aspect_mode = vi; core_video_changed = true; } }
	CFG_INT("hatarib_samplerate") newparam.Sound.nPlaybackFreq = vi;
	CFG_INT("hatarib_ymmix") newparam.Sound.YmVolumeMixing = vi;
	CFG_INT("hatarib_lpf") newparam.Sound.YmLpf = vi;
	CFG_INT("hatarib_hpf") newparam.Sound.YmHpf = vi;
	CFG_INT("hatarib_driveb") { newparam.DiskImage.EnableDriveB = vi; core_disk_enable_b = vi; }
	CFG_INT("hatarib_drivesingle") { newparam.DiskImage.DriveA_NumberOfHeads = newparam.DiskImage.DriveB_NumberOfHeads = vi; }
	CFG_INT("hatarib_readonly_floppy") { newparam.DiskImage.nWriteProtection = vi; if (vi) core_disk_save = false; }
	CFG_INT("hatarib_patchtos") newparam.System.bFastBoot = vi;
	CFG_INT("hatarib_blitter_st") newparam.System.bBlitter = vi;
	CFG_INT("hatarib_wakestate") newparam.System.VideoTimingMode = vi;
	CFG_INT("hatarib_prefetch") newparam.System.bCompatibleCpu = vi;
	CFG_INT("hatarib_cycle_exact") newparam.System.bCycleExactCpu = vi;
	CFG_INT("hatarib_mmu") newparam.System.bMMU = vi;
	for (int i=0; i<4; ++i)
	{
		CFG_INT_PAD(i,"dpad"  ) core_stick_map[ i][CORE_INPUT_STICK_DPAD   ] = vi;
		CFG_INT_PAD(i,"a"     ) core_button_map[i][CORE_INPUT_BUTTON_A     ] = vi;
		CFG_INT_PAD(i,"b"     ) core_button_map[i][CORE_INPUT_BUTTON_B     ] = vi;
		CFG_INT_PAD(i,"x"     ) core_button_map[i][CORE_INPUT_BUTTON_X     ] = vi;
		CFG_INT_PAD(i,"y"     ) core_button_map[i][CORE_INPUT_BUTTON_Y     ] = vi;
		CFG_INT_PAD(i,"select") core_button_map[i][CORE_INPUT_BUTTON_SELECT] = vi;
		CFG_INT_PAD(i,"start" ) core_button_map[i][CORE_INPUT_BUTTON_START ] = vi;
		CFG_INT_PAD(i,"l1"    ) core_button_map[i][CORE_INPUT_BUTTON_L1    ] = vi;
		CFG_INT_PAD(i,"r1"    ) core_button_map[i][CORE_INPUT_BUTTON_R1    ] = vi;
		CFG_INT_PAD(i,"l2"    ) core_button_map[i][CORE_INPUT_BUTTON_L2    ] = vi;
		CFG_INT_PAD(i,"r2"    ) core_button_map[i][CORE_INPUT_BUTTON_R2    ] = vi;
		CFG_INT_PAD(i,"l3"    ) core_button_map[i][CORE_INPUT_BUTTON_L3    ] = vi;
		CFG_INT_PAD(i,"r3"    ) core_button_map[i][CORE_INPUT_BUTTON_R3    ] = vi;
		CFG_INT_PAD(i,"lstick") core_stick_map[ i][CORE_INPUT_STICK_L      ] = vi;
		CFG_INT_PAD(i,"rstick") core_stick_map[ i][CORE_INPUT_STICK_R      ] = vi;
		CFG_INT_PAD(i,"oskey_confirm") core_oskey_map[i][CORE_INPUT_OSKEY_CONFIRM] = vi; // buttons
		CFG_INT_PAD(i,"oskey_cancel" ) core_oskey_map[i][CORE_INPUT_OSKEY_CANCEL ] = vi;
		CFG_INT_PAD(i,"oskey_shift"  ) core_oskey_map[i][CORE_INPUT_OSKEY_SHIFT  ] = vi;
		CFG_INT_PAD(i,"oskey_pos"    ) core_oskey_map[i][CORE_INPUT_OSKEY_POS    ] = vi;
		CFG_INT_PAD(i,"oskey_move"   ) core_oskey_map[i][CORE_INPUT_OSKEY_MOVE   ] = vi; // stick
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
	retro_log(RETRO_LOG_DEBUG,"core_config_set_environment(%p)\n",cb);
	bool tos_img = false;

	if ((def = get_core_option_def("hatarib_tos")))
	{
		int i = 0;
		int j = 0;
		if (core_file_system_count() > 0 && !strcmp(core_file_system_filename(0),"tos.img"))
		{
			// default to system/tos.img if you have it
			def->default_value = "<tos.img>";
			tos_img = true;
			++i;
		}
		else
		{
			// otherwise indicate that it's missing
			def->values[0].label = "system/tos.img (missing)";
		}
		
		for (; (i<core_file_system_count()) && (j<MAX_OPTION_FILES); ++i)
		{
			const char* fn = core_file_system_filename(i);
			if (!has_extension(fn,"img\0" "rom\0" "bin\0" "tos\0" "\0")) continue;
			def->values[j+4].value = fn;
			def->values[j+4].label = fn + 8; // hatarib/
			++j;
		}
	}
	if ((def = get_core_option_def("hatarib_cartridge")))
	{
		int i = tos_img ? 1 : 0;
		int j = 0;
		for (; (i<core_file_system_count()) && (j<MAX_OPTION_FILES); ++i)
		{
			const char* fn = core_file_system_filename(i);
			if (!has_extension(fn,"img\0" "rom\0" "bin\0" "cart\0" "\0")) continue;
			def->values[j+1].value = fn;
			def->values[j+1].label = fn + 8; // hatarib/
			++j;
		}
	}
	if ((def = get_core_option_def("hatarib_hardimg")))
	{
		int i = 0;
		int j = 0;
		// directories
		for (; (i<core_file_system_dir_count()) && (j<MAX_OPTION_FILES); ++j, ++i)
		{
			def->values[j+1].value = core_file_system_dirname(i);
			def->values[j+1].label = core_file_system_dirlabel(i);
			++j;
		}
		// files
		i = tos_img ? 1 : 0;
		for (; (i<core_file_system_count()) && (j<MAX_OPTION_FILES); ++j, ++i)
		{
			const char* fn = core_file_system_filename(i);
			// What are appropriate extensions? Hatari seems to allow anything.
			if (!has_extension(fn,"img\0" "acsi\0" "scsi\0" "bin\0" "\0")) continue;
			def->values[j+1].value = fn;
			def->values[j+1].label = fn + 8; // hatarib/
			++j;
		}
	}

	if (!cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version)) version = 0;
	//version = 1; // to test version 1
	//version = 0; // to test version 0
	if (version >= 2)
	{
		cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, (void*)&CORE_OPTIONS_V2);
	}
	else if (version >= 1) // fallback to version 1
	{
		struct retro_core_option_definition* dv1 = CORE_OPTIONS_V1;
		for (def = CORE_OPTION_DEF; def->key != NULL; ++def, ++dv1)
		{
			dv1->key = def->key;
			dv1->desc = def->desc;
			dv1->info = def->info;
			for (int i=0; i<RETRO_NUM_CORE_OPTION_VALUES_MAX; ++i)
				dv1->values[i] = def->values[i];
			dv1->default_value = def->default_value;
		}
		cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, (void*)&CORE_OPTIONS_V1);
	}
	else // fallback to version 0
	{
		struct retro_variable* dv0 = CORE_OPTIONS_V0;
		for (def = CORE_OPTION_DEF; def->key != NULL; ++def, ++dv0)
		{
			dv0->key = def->key;
			dv0->value = def->default_value;
		}
		cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)&CORE_OPTIONS_V0);
	}

	cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)INPUT_DESCRIPTORS);
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
