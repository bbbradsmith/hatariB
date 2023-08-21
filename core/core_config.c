#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include "../hatari/src/includes/main.h"
#include "../hatari/src/includes/configuration.h"

static CNF_PARAMS defparam;
static CNF_PARAMS newparam;

// system/hatarib/ file scane used to populate arrays in CORE_OPTION_DEF below
#define MAX_OPTION_FILES   100

const char* const NUMBERS[128] = {
	  "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9", "10", "11", "12", "13", "14", "15",
	 "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31",
	 "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47",
	 "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61", "62", "63",
	 "64", "65", "66", "67", "68", "69", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
	 "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "90", "91", "92", "93", "94", "95",
	 "96", "97", "98", "99","100","101","102","103","104","105","106","107","108","109","110","111",
	"112","113","114","115","116","117","118","119","120","121","122","123","124","125","126","127",
};

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
			{NULL,NULL}
		}, "1"
	},
	{
		"hatarib_fast_floppy", "Fast Floppy", NULL,
		"Artifically accelerate floppy disk access, reducing load times.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_save_floppy", "Save Floppy Disks", NULL,
		"Changes to floppy disks will save a copy in saves/."
		" If turned off, changes will be lost when the content is closed.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_soft_reset", "Soft Reset", NULL,
		"Core Restart is full cold boot by default (power off, on),"
		" but this will change it to a warm boot (reset button).",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
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
		"Causes restart!! Atari ST memory size.",
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
		"Causes restart!! Hard drive image, list of files and directories from system/hatarib/.",
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
		"hatarib_hardtype", "Hard Disk Type", NULL,
		"Causes restart!! GemDOS type will simulate a hard disk from a folder in system/hatarib/."
		" The other types must use an image file.",
		NULL, "system",
		{
			{"0","GemDOS"},
			{"1","GemDOS (Use 8-bit Filenames)"},
			{"2","ACSI"},
			{"3","SCSI"},
			{"4","IDE (Auto)"},
			{"5","IDE (Byte Swap Off)"},
			{"6","IDE (Byte Swap On)"},
			{NULL,NULL},
		}, "0"
	},
	{
		"hatarib_hardboot", "Hard Disk Boot", NULL,
		"Boot from hard disk.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
	},
	{
		"hatarib_hard_readonly", "Hard Disk Write Protect", NULL,
		"Write protect the hard disk folder or image.",
		NULL, "system",
		{{"0","Off"},{"1","On"},{"2","Auto"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_emutos_region", "EmuTOS Region", NULL,
		"Causes restart!! For EmuTOS ROMs this can override the default framerate."
		" EmuTOS 1024k can choose a default region, which sets language, keyboard and framerate together.",
		NULL, "system",
		{
			{"-1","Default"},
			{"-2","NTSC 60 Hz"},
			{"-3","PAL 50 Hz"},
			{"0","USA (NTSC)"},
			{"1","Germany"},
			{"2","France"},
			{"3","United Kingdom"},
			{"4","Spain"},
			{"5","Italy"},
			{"6","Sweden"},
			{"7","Switzerland (French)"},
			{"8","Swizterland (German)"},
			{"9","Turkey"},
			{"10","Finland"},
			{"11","Norway"},
			{"12","Denmark"},
			{"13","Saudi Arabia"},
			{"14","Netherlands"},
			{"15","Czech Republic"},
			{"16","Hungary"},
			{"17","Poland"},
			{"19","Russia"},
			{"31","Greece"},
			{"127","Multilanguage"},
			{NULL,NULL},
		}, "0"
	},
	//
	// Input
	//
	{ "hatarib_joy1_port", "Joystick 1", NULL, "Retropad 1 assigned Atari port.", NULL, "input", {{"6","None"},{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"},{NULL,NULL}},"1" },
	{ "hatarib_joy2_port", "Joystick 2", NULL, "Retropad 2 assigned Atari port.", NULL, "input", {{"6","None"},{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"},{NULL,NULL}},"0" },
	{ "hatarib_joy3_port", "Joystick 3", NULL, "Retropad 3 assigned Atari port.", NULL, "input", {{"6","None"},{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"},{NULL,NULL}},"2" },
	{ "hatarib_joy4_port", "Joystick 4", NULL, "Retropad 4 assigned Atari port.", NULL, "input", {{"6","None"},{"0","Joy 0"},{"1","Joy 1"},{"2","STE A"},{"3","STE B"},{"4","Parallel 1"},{"5","Parallel 2"},{NULL,NULL}},"3" },
	{
		"hatarib_mouse_port", "Mouse", NULL,
		"Mouse connected to Joy 0 port."
		" This can be connected at the same time as a joystick, but their inputs will overlap.",
		NULL, "input",
		{{"0","None"},{"1","Joy 0"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_host_mouse", "Host Mouse Enabled", NULL,
		"Allow input from your own mouse device."
		" With this disabled you can still use the retropad mouse inputs.",
		NULL, "input",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_host_keyboard", "Host Keyboard Enabled", NULL,
		"Allow input from your own keyboard."
		" With this disabled you can still use the onscreen keyboard or retropad mapped keys.",
		NULL, "input",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
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
			{"2","1"}, // 2 is the minimum 1px for d-pad motion with the current factors (32768 * 0.00005 * 0.4 * 2 = ~1.3 >= 1)
			{"3","2"},
			{"4","3"},
			{"5","4"},
			{"6","5"}, // 6 seems comfortable
			{"7","6"},
			{"8","7"},
			{"10","8"},
			{"12","9"},
			{"14","10"}, // giving it a slight curve, but calling it 1-10 for the user
			{NULL,NULL},
		}, "6"
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
	{
		"hatarib_osk_press_len", "On-Screen Keyboard Press Time", NULL,
		"How many frames to apply a button press from the on-scren keyboard",
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
			{"15","15"},
			{"20","20"},
			{"25","25"},
			{"30","30"},
			{"35","35"},
			{"40","40"},
			{"45","45"},
			{"50","50"},
			{"55","55"},
			{"60","60"},
			{"65","65"},
			{"70","70"},
			{"71","71"},
			{NULL,NULL},
		}, "5"
	},
	//{ // disabled pending more information
	//	"hatarib_osk_layout", "On-Screen Keyboard Layout", NULL,
	//	"Choose a layout for the on-screen keyboard.",
	//	NULL,"input",
	//	{
	//		{"0","QWERTY"},
	//		//{"1","QWERTZ"},
	//		//{"2","AZERTY"},
	//		// https://tho-otto.de/keyboards/
	//		// BOOTCONF utility gave: USA, D, F,GB, E, I , S, CH (F), CH (D)
	//		// I think just these 3 might be sufficient though?
	//		// Need more information about how to test this, and how keys like <> should be mapped.
	//	}, "0"
	//},
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
		}, "0"
	},
	{
		"hatarib_pause_osk", "Pause Screen Display", NULL,
		"The help screen is displayed at pause by default, but there are alternatives.",
		NULL, "video",
		{
			{"2","Help and Information"},
			{"3","Floppy Disk List"},
			{"4","Bouncing Box"},
			{"5","Snow"},
			{"0","Darken"},
			{"1","No Indicator"},
			{NULL,NULL}
		}, "2"		
	},
	{
		"hatarib_show_welcome", "Show Welcome Message", NULL,
		"At startup the status bar shows a welcome message for 5 seconds, if enabled.",
		NULL, "video",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
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
			{"1","Hatari STF"},
			{"2","Hatari STE/Falcon"},
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
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_drivesingle","Single-Sided Drives", NULL,
		"Single-Sided floppy drives instead of Double-Sided.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
	},
	{
		"hatarib_readonly_floppy","Write Protect Floppy Disks", NULL,
		"Write-protect all floppy disks in emulation."
		" The emulated operating system will know that all writes are failing.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
	},
	{
		"hatarib_cpu", "CPU", NULL,
		"Causes restart!! 68000 family CPU type.",
		NULL, "advanced",
		{
			{"-1","Auto"},
			{"0","68000"},
			{"1","68010"},
			{"2","68020"},
			{"3","68030"},
			{"4","68040"},
			{"5","68060"}, // no 68050 eh?
			{NULL,NULL}
		}, "-1",
	},
	{
		"hatarib_cpu_clock", "CPU Clock Rate", NULL,
		"CPU speed at boot.",
		NULL, "advanced",
		{
			{"-1","Auto"},
			{"8","8 MHz"},
			{"16","16 MHz"},
			{"32","32 MHz"},
			{NULL,NULL}
		}, "-1",
	},
	{
		"hatarib_fpu", "FPU", NULL,
		"Causes restart!! Floating point unit used with the CPU.",
		NULL, "advanced",
		{
			{"-1","Auto"},
			{"0","None"},
			{"68881","68881"},
			{"68882","68882"},
			{"68040","Internal"},
			{NULL,NULL}
		}, "-1",
	},
	{
		"hatarib_patchtos","Patch TOS for Fast Boot", NULL,
		"Boot slightly faster for some known TOS ROMs.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_crashtime","Crash Timeout Reset", NULL,
		"Time in seconds. If the CPU halts, nothing will happen until a hard reset."
		" This option will automatically reset after the chosen time.",
		NULL, "advanced",
		{
			{"0","Off"},
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
			{"25","25"},
			{"30","30"},
			{"35","35"},
			{"40","40"},
			{"45","45"},
			{"50","50"},
			{"55","55"},
			{"60","60"},
			{NULL,NULL}
		}, "10"
	},
	{
		"hatarib_blitter_st","Blitter in ST Mode", NULL,
		"Causes restart!! Normally the blitter requires a Mega ST.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
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
		"Causes restart!! Uses more CPU power, more accurate, commonly needed.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_cycle_exact", "Cycle-exact Cache Emulation", NULL,
		"Causes restart!! Uses more CPU power, more accurate.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
	},
	{
		"hatarib_mmu", "MMU Emulation", NULL,
		"Causes restart!! For TT or Falcon. Uses more CPU power.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
	},
	#if CORE_INPUT_DEBUG
	{
		"hatarib_input_debug", "Input Debug Log", NULL,
		"For debugging input, dump polled inputs to the log every frame.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
	},
	#endif
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
	// The values for OPTION_PAD_BUTTON will be automatically replaced with NUMBERS.
	// The ones that are numbered at the beginning are for reference when implementing
	// their mapping in core_input.c (which must match this list precisely)
	// and also make sure the defaults still match (e.g. key space / key return on L3/R3)
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
		{"8","Help Screen / Pause"}, \
		{"9","Joystick Up"}, \
		{"10","Joystick Down"}, \
		{"11","Joystick Left"}, \
		{"12","Joystick Right"}, \
		{"13","STE Button A"}, \
		{"14","STE Button B"}, \
		{"15","STE Button C"}, \
		{"16","STE Button Option"}, \
		{"17","STE Button Pause"}, \
		{"18","Soft Reset"}, \
		{"19","Hard Reset"}, \
		{"20","Toggle Status Bar"}, \
		{"21","Key Space"}, \
		{"22","Key Return"}, \
		{"","Key Up"}, \
		{"","Key Down"}, \
		{"","Key Left"}, \
		{"","Key Right"}, \
		{"","Key F1"}, \
		{"","Key F2"}, \
		{"","Key F3"}, \
		{"","Key F4"}, \
		{"","Key F5"}, \
		{"","Key F6"}, \
		{"","Key F7"}, \
		{"","Key F8"}, \
		{"","Key F9"}, \
		{"","Key F10"}, \
		{"","Key Esc"}, \
		{"","Key 1"}, \
		{"","Key 2"}, \
		{"","Key 3"}, \
		{"","Key 4"}, \
		{"","Key 5"}, \
		{"","Key 6"}, \
		{"","Key 7"}, \
		{"","Key 8"}, \
		{"","Key 9"}, \
		{"","Key 0"}, \
		{"","Key Minus"}, \
		{"","Key Equals"}, \
		{"","Key Backquote"}, \
		{"","Key Backspace"}, \
		{"","Key Tab"}, \
		{"","Key Q"}, \
		{"","Key W"}, \
		{"","Key E"}, \
		{"","Key R"}, \
		{"","Key T"}, \
		{"","Key Y"}, \
		{"","Key U"}, \
		{"","Key I"}, \
		{"","Key O"}, \
		{"","Key P"}, \
		{"","Key Left Brace"}, \
		{"","Key Right Brace"}, \
		{"","Key Delete"}, \
		{"","Key Control"}, \
		{"","Key A"}, \
		{"","Key S"}, \
		{"","Key D"}, \
		{"","Key F"}, \
		{"","Key G"}, \
		{"","Key H"}, \
		{"","Key J"}, \
		{"","Key K"}, \
		{"","Key L"}, \
		{"","Key Semicolon"}, \
		{"","Key Quote"}, \
		{"","Key Backslash"}, \
		{"","Key Left Shift"}, \
		{"","Key Z"}, \
		{"","Key X"}, \
		{"","Key C"}, \
		{"","Key V"}, \
		{"","Key B"}, \
		{"","Key N"}, \
		{"","Key M"}, \
		{"","Key Comma"}, \
		{"","Key Period"}, \
		{"","Key Slash"}, \
		{"","Key Right Shift"}, \
		{"","Key Alternate"}, \
		{"","Key Capslock"}, \
		{"","Key Help"}, \
		{"","Key Undo"}, \
		{"","Key Insert"}, \
		{"","Key Clr Home"}, \
		{"","Key Numpad Left Paren"}, \
		{"","Key Numpad Right Paren"}, \
		{"","Key Numpad Divide"}, \
		{"","Key Numpad Multiply"}, \
		{"","Key Numpad Subtract"}, \
		{"","Key Numpad Add"}, \
		{"","Key Numpad Enter"}, \
		{"","Key Numpad Decimal"}, \
		{"","Key Numpad 0"}, \
		{"","Key Numpad 1"}, \
		{"","Key Numpad 2"}, \
		{"","Key Numpad 3"}, \
		{"","Key Numpad 4"}, \
		{"","Key Numpad 5"}, \
		{"","Key Numpad 6"}, \
		{"","Key Numpad 7"}, \
		{"","Key Numpad 8"}, \
		{"","Key Numpad 9"}, \
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
		OPTION_PAD_BUTTON(), "21" }, /* key space */ \
	{ "hatarib_pad" padnum "_r3", "Pad " padnum " R3", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_BUTTON(), "22" }, /* key return */ \
	{ "hatarib_pad" padnum "_lstick", "Pad " padnum " Left Analog Stick", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_STICK(), "1" }, /* joystick */ \
	{ "hatarib_pad" padnum "_rstick", "Pad " padnum " Right Analog Stick", NULL, NULL, NULL, "pad" padnum, \
		OPTION_PAD_STICK(), "2" }, /* mouse */ \
	{ "hatarib_pad" padnum "_oskey_confirm", "Pad " padnum " On-Screen Key Confirm", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "7" }, /* l1 */ \
	{ "hatarib_pad" padnum "_oskey_cancel", "Pad " padnum " On-Screen Key Cancel", NULL, NULL, NULL, "pad" padnum, \
		OPTION_OSKEY_BUTTON(), "8" }, /* r1 */ \
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
extern void Screen_ModeChanged(bool bForceChange);

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

const char* cfg_get_pad_key(int pad, const char* key)
{
	static char padkey[64];
	snprintf(padkey,sizeof(padkey),"hatarib_pad%d_%s",pad+1,key);
	padkey[sizeof(padkey)-1] = 0;
	return padkey;
}

bool cfg_read_int_pad(int pad, const char* key, int* v)
{
	return cfg_read_int(cfg_get_pad_key(pad,key),v);
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
	CFG_INT("hatarib_save_floppy") core_disk_enable_save = vi;
	CFG_INT("hatarib_soft_reset") core_option_soft_reset = vi;
	CFG_INT("hatarib_machine")
	{
		// automatic setup based on OPT_MACHINE in options.c
		static const int CPULEVEL[] = { 0, 0, 0, 0, 3, 3 };
		static const int CPUFREQ[]  = { 8, 8, 8, 8,32,16 };
		newparam.System.nMachineType = vi;
		newparam.System.nCpuLevel = CPULEVEL[vi];
		newparam.System.nBootCpuFreq = CPUFREQ[vi];
		if (vi == MACHINE_TT)
		{
			newparam.System.bCompatibleFPU = true;
			newparam.System.n_FPUType = FPU_68882;
		}
		else if (vi == MACHINE_FALCON)
		{
			newparam.System.nDSPType = DSP_TYPE_EMU;
		}
		CFG_INT("hatarib_cpu") if (vi>=0) newparam.System.nCpuLevel = vi;
		CFG_INT("hatarib_cpu_clock") if (vi>=0) newparam.System.nBootCpuFreq = vi;
		CFG_INT("hatarib_fpu") if (vi>=0) newparam.System.n_FPUType = vi;
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
			case 0: // GemDOS
			case 1: // GemDOS (Use 8-bit Filenames)
				newparam.HardDisk.bFilenameConversion = (ht == 1);
				newparam.HardDisk.bUseHardDiskDirectories = true;
				strcpy_trunc(newparam.HardDisk.szHardDiskDirectories[0],image,sizeof(newparam.HardDisk.szHardDiskDirectories[0]));
				break;
			case 2: // ACSI
				newparam.Acsi[0].bUseDevice = true;
				strcpy_trunc(newparam.Acsi[0].sDeviceFile,image,sizeof(newparam.Acsi[0].sDeviceFile));
				break;
			case 3: // SCSI
				newparam.Scsi[0].bUseDevice = true;
				strcpy_trunc(newparam.Scsi[0].sDeviceFile,image,sizeof(newparam.Scsi[0].sDeviceFile));
				break;
			case 4: // IDE (Auto)
			case 5: // IDE (Byte Swap Off)
			case 6: // IDE (Byte Swap On)
				newparam.Ide[0].bUseDevice = true;
				strcpy_trunc(newparam.Ide[0].sDeviceFile,image,sizeof(newparam.Ide[0].sDeviceFile));
				{
					static const BYTESWAPPING BSMAP[3] = { BYTESWAP_AUTO, BYTESWAP_OFF, BYTESWAP_ON };
					newparam.Ide[0].nByteSwap = BSMAP[ht-4];
				}
				break;
			}
		}
	}
	CFG_INT("hatarib_hardboot") newparam.HardDisk.bBootFromHardDisk = vi;
	CFG_INT("hatarib_hard_readonly") { newparam.HardDisk.nWriteProtection = vi; core_hard_readonly = vi; }
	CFG_INT("hatarib_emutos_region")
	{
		if (vi >= 0) newparam.Rom.nEmuTosRegion = vi;
		if (vi == -2) newparam.Rom.nEmuTosFramerate = 0; // NTSC 60 Hz
		if (vi == -3) newparam.Rom.nEmuTosFramerate = 1; // PAL 50 Hz
	}
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
	//CFG_INT("hatarib_osk_layout") core_osk_layout = vi; // disabled pending more information
	CFG_INT("hatarib_osk_press_len") core_osk_press_len = vi;
	CFG_INT("hatarib_lowres2x") newparam.Screen.bLowResolutionDouble = vi;
	CFG_INT("hatarib_borders") newparam.Screen.bAllowOverscan = vi;
	CFG_INT("hatarib_statusbar") { newparam.Screen.bShowStatusbar = (vi==1); newparam.Screen.bShowDriveLed = (vi==2); }
	CFG_INT("hatarib_aspect") { if (core_video_aspect_mode != vi) { core_video_aspect_mode = vi; core_video_changed = true; } }
	CFG_INT("hatarib_pause_osk") core_pause_osk = vi;
	CFG_INT("hatarib_show_welcome") core_show_welcome = vi;
	CFG_INT("hatarib_samplerate") newparam.Sound.nPlaybackFreq = vi;
	CFG_INT("hatarib_ymmix") newparam.Sound.YmVolumeMixing = vi;
	CFG_INT("hatarib_lpf") newparam.Sound.YmLpf = vi;
	CFG_INT("hatarib_hpf") newparam.Sound.YmHpf = vi;
	CFG_INT("hatarib_driveb") { newparam.DiskImage.EnableDriveB = vi; core_disk_enable_b = vi; }
	CFG_INT("hatarib_drivesingle") { newparam.DiskImage.DriveA_NumberOfHeads = newparam.DiskImage.DriveB_NumberOfHeads = vi; }
	CFG_INT("hatarib_readonly_floppy") newparam.DiskImage.nWriteProtection = vi;
	//CFG_INT("hatarib_cpu") // handle within machine
	//CFG_INT("hatarib_cpu_clock") // handle within machine
	//CFG_INT("hatarib_fpu") // handle within macine
	CFG_INT("hatarib_patchtos") newparam.System.bFastBoot = vi;
	CFG_INT("hatarib_crashtime") core_crashtime = vi;
	CFG_INT("hatarib_blitter_st") newparam.System.bBlitter = vi;
	CFG_INT("hatarib_wakestate") newparam.System.VideoTimingMode = vi;
	CFG_INT("hatarib_prefetch") newparam.System.bCompatibleCpu = vi;
	CFG_INT("hatarib_cycle_exact") newparam.System.bCycleExactCpu = vi;
	CFG_INT("hatarib_mmu") newparam.System.bMMU = vi;
	#if CORE_INPUT_DEBUG
		CFG_INT("hatarib_input_debug") core_input_debug = vi;
	#endif
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
		CFG_INT_PAD(i,"oskey_pos"    ) core_oskey_map[i][CORE_INPUT_OSKEY_POS    ] = vi;
		CFG_INT_PAD(i,"oskey_move"   ) core_oskey_map[i][CORE_INPUT_OSKEY_MOVE   ] = vi; // stick
	}

	// preserve parameters that change during emulation
	memcpy(newparam.DiskImage.szDiskFileName[0],ConfigureParams.DiskImage.szDiskFileName[0],sizeof(newparam.DiskImage.szDiskFileName[0]));
	memcpy(newparam.DiskImage.szDiskFileName[1],ConfigureParams.DiskImage.szDiskFileName[1],sizeof(newparam.DiskImage.szDiskFileName[1]));
	newparam.System.nCpuFreq = ConfigureParams.System.nCpuFreq;
}

void config_toggle_statusbar(void)
{
	ConfigureParams.Screen.bShowStatusbar = !ConfigureParams.Screen.bShowStatusbar;
	Screen_ModeChanged(true);
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
	//retro_log(RETRO_LOG_DEBUG,"core_config_set_environment(%p)\n",cb);
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
		for (; (i<core_file_system_dir_count()) && (j<MAX_OPTION_FILES); ++i)
		{
			def->values[j+1].value = core_file_system_dirname(i);
			def->values[j+1].label = core_file_system_dirlabel(i);
			++j;
		}
		// files
		i = tos_img ? 1 : 0;
		for (; (i<core_file_system_count()) && (j<MAX_OPTION_FILES); ++i)
		{
			const char* fn = core_file_system_filename(i);
			// What are appropriate extensions? Hatari seems to allow anything.
			def->values[j+1].value = fn;
			def->values[j+1].label = fn + 8; // hatarib/
			++j;
		}
	}
	for (int i=0; i<4; ++i)
	{
		// fill in the number values for OPTION_PAD_BUTTONs
		static const char* const BUTTONS[12] =  {"a","b","x","y","select","start","l1","l2","r1","r2","l3","r3"};
		for (int j=0; j<12; ++j)
		{
			const char* key = cfg_get_pad_key(i,BUTTONS[j]);
			if((def = get_core_option_def(key)))
			{
				for (int k=0; k<128 && def->values[k].label != NULL; ++k)
				{
					def->values[k].value = NUMBERS[k];
					// for debugging the list
					//retro_log(RETRO_LOG_DEBUG,"%s[%d] = %s (%s)\n",key,k,def->values[k].value,def->values[k].label);
				}
			}
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
	if (reset) // boot configurations that need to be applied at reset
	{
		newparam.System.nCpuFreq = newparam.System.nBootCpuFreq;
	}
	Change_CopyChangedParamsToConfiguration(&ConfigureParams, &newparam, reset);
}

void core_config_reset(void) // boot configurations that need to be applied at reset
{
	ConfigureParams.System.nCpuFreq = ConfigureParams.System.nBootCpuFreq;
}
