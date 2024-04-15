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
		}, "<etos192uk>" // replace with <tos.img> if system/tos.img exists
	},
	{
		"hatarib_monitor", "Monitor", NULL,
		"Causes restart!! Monitor type. Colour, Monochrome, VGA, TV."
		" TV scanlines effect requires doubled low/medium resolution.",
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
		"hatarib_savestate_floppy_modify", "Floppy Savestate Safety Save", NULL,
		"Disable this for netplay or run-ahead. "
		"Modified floppies are always saved during eject or content closing, "
		" but this setting produces an extra save before/after restoring a savestate to prevent un-ejected data loss."
		" Because netplay and run-ahead use savestates constantly, this should be turned off for those activities.",
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
		"hatarib_emutos_framerate", "EmuTOS Framerate", NULL,
		"Causes restart!! For EmuTOS ROMs this can override the default framerate.",
		NULL, "system",
		{
			{"-1","Default"},
			{"0","NTSC 60 Hz"},
			{"1","PAL 50 Hz"},
			{NULL,NULL},
		}, "-1"
	},
	{
		"hatarib_emutos_region", "EmuTOS 1024k Region", NULL,
		"Causes restart!! EmuTOS 1024k can choose a default region, which sets language and keyboard.",
		NULL, "system",
		{
			{"-1","Default"},
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
		}, "-1"
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
			{"2","1"}, // 2 is very slow
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
		"hatarib_osk_layout", "On-Screen Keyboard Layout", NULL,
		"Choose a language layout for the on-screen keyboard.",
		NULL,"input",
		{ // Source: https://tho-otto.de/keyboards/
			{"0","US QWERTY"},
			{"1","German QWERTZ"},
			{"2","French AZERTY"},
			{"3","UK QWERTY"},
			{"4","Spanish QWERTY"},
			{"5","Italian QWERTY"},
			{"6","Swedish QWERTY"},
			{"7","Swiss French QWERTZ"},
			{"8","Swiss German QWERTZ"},
			{"9","Finnish QWERTY"},
			{"10","Norwegian QWERTY"},
			{"11","Danish QWERTY"},
			{"12","Dutch QWERTY"},
			{"13","Czech QWERTZ"},
			{"14","Hungarian QWERTZ"},
			{"15","Polish QWERTY"},
		}, "0"
	},
	{
		"hatarib_osk_press_len", "On-Screen Keyboard Press Time", NULL,
		"Minimum number of frames to apply a button press from the on-screen keyboard",
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
	{
		"hatarib_osk_repeat_delay", "On-Screen Keyboard Repeat Delay", NULL,
		"Holding a direction will repeat moves after this amount of time.",
		NULL, "input",
		{
			{"50","50 ms"},
			{"75","75 ms"},
			{"100","100 ms"},
			{"125","125 ms"},
			{"150","150 ms"},
			{"175","175 ms"},
			{"200","200 ms"},
			{"225","225 ms"},
			{"250","250 ms"},
			{"275","275 ms"},
			{"300","300 ms"},
			{"325","325 ms"},
			{"350","350 ms"},
			{"375","375 ms"},
			{"400","400 ms"},
			{"425","425 ms"},
			{"450","450 ms"},
			{"475","475 ms"},
			{"500","500 ms"},
			{"550","550 ms"},
			{"600","600 ms"},
			{"650","650 ms"},
			{"700","700 ms"},
			{"750","750 ms"},
			{"800","800 ms"},
			{"850","850 ms"},
			{"900","900 ms"},
			{"950","950 ms"},
			{"1000","1000 ms"},
			{"1100","1100 ms"},
			{"1200","1200 ms"},
			{"1300","1300 ms"},
			{"1400","1400 ms"},
			{"1500","1500 ms"},
			{"1600","1600 ms"},
			{"1700","1700 ms"},
			{"1800","1800 ms"},
			{"1900","1900 ms"},
			{"2000","2000 ms"},
			{"1200","2200 ms"},
			{"1400","2400 ms"},
			{"1600","2600 ms"},
			{"1800","2800 ms"},
			{"3000","3000 ms"},
			{"0","Off"},
			{NULL,NULL},
		}, "500"
	},
	{
		"hatarib_osk_repeat_rate", "On-Screen Keyboard Repeat Rate", NULL,
		"Holding a direction will repeat moves at this rate after the first delay.",
		NULL, "input",
		{
			{"50","50 ms"},
			{"75","75 ms"},
			{"100","100 ms"},
			{"125","125 ms"},
			{"150","150 ms"},
			{"175","175 ms"},
			{"200","200 ms"},
			{"225","225 ms"},
			{"250","250 ms"},
			{"275","275 ms"},
			{"300","300 ms"},
			{"325","325 ms"},
			{"350","350 ms"},
			{"375","375 ms"},
			{"400","400 ms"},
			{"425","425 ms"},
			{"450","450 ms"},
			{"475","475 ms"},
			{"500","500 ms"},
			{"550","550 ms"},
			{"600","600 ms"},
			{"650","650 ms"},
			{"700","700 ms"},
			{"750","750 ms"},
			{"800","800 ms"},
			{"850","850 ms"},
			{"900","900 ms"},
			{"950","950 ms"},
			{"1000","1000 ms"},
			{"1100","1100 ms"},
			{"1200","1200 ms"},
			{"1300","1300 ms"},
			{"1400","1400 ms"},
			{"1500","1500 ms"},
			{"1600","1600 ms"},
			{"1700","1700 ms"},
			{"1800","1800 ms"},
			{"1900","1900 ms"},
			{"2000","2000 ms"},
			{"1200","2200 ms"},
			{"1400","2400 ms"},
			{"1600","2600 ms"},
			{"1800","2800 ms"},
			{"3000","3000 ms"},
			{NULL,NULL},
		}, "150"
	},
	//
	// Video
	//
	{
		"hatarib_res2x", "Resolution Double", NULL,
		"Doubles pixels for low and/or medium resolution, "
		" Prevents video output size changes for resolution switch,"
		" and keeps the medium resolution PAR closer to square.",
		NULL, "video",
		{
			{"0","Off"},
			{"1","Double Medium"},
			{"2","Double Low + Medium"},
			{NULL,NULL},
		}, "1"
	},
	{
		"hatarib_borders", "Screen Borders", NULL,
		"Atari ST monitors had a visible border around the main screen area,"
		" but most software does not display anything in it.",
		NULL, "video",
		{
			{"0","None"},
			{"1","Small"},
			{"2","Medium"},
			{"3","Large"},
			{"4","Maximum"},
			{"5","Crop 720p (240, 480)"},
			{"6","Crop 1080p (270, 540)"},
			{NULL,NULL}
		}, "2"
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
			{"4","4:3"},
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
	{
		"hatarib_boot_alert", "Boot Notification", NULL,
		"Show notification for reset/reboot.",
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
		"Reduces high frequency noise from sound output to reduce harshness.",
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
	{
		"hatarib_midi", "MIDI Enable", NULL,
		"MIDI I/O is enabled by default if you have a MIDI device set,"
		" but it can be disabled here.",
		NULL, "audio",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
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
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "1"
	},
	{
		"hatarib_mmu", "MMU Emulation", NULL,
		"Causes restart!! For TT or Falcon. Uses more CPU power.",
		NULL, "advanced",
		{{"0","Off"},{"1","On"},{NULL,NULL}}, "0"
	},
	{
		"hatarib_log_hatari", "Hatari Logging", NULL,
		"Hatari's internal log messages can be sent to the RetroArch logs."
		" Requires content close and re-open.",
		NULL, "advanced",
		{
			{"0","Fatal"},
			{"1","Error"},
			{"2","Warn"},
			{"3","Info"},
			{"4","To Do"},
			{"5","Debug"},
			{NULL,NULL}
		}, "1",
	},
	{
		"hatarib_perf_counters", "Performance Counters", NULL,
		"Display performance timing on the status bar: "
		"frame (average) + last: reset, savestate, restore (μs)",
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
	// OPTION_PAD defined in core_internal.h
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
extern void Statusbar_UpdateInfo(void);

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

static void core_config_hard(CNF_PARAMS *param, const char* image, int ht)
{
	int i = 0;
	retro_log(RETRO_LOG_INFO,"core_config_hard(%p,'%s',%d)\n",param,image,ht);

	// set new drive
	switch(ht)
	{
	default:
	case 0: // GemDOS
	case 1: // GemDOS (Use 8-bit Filenames)
		if (param->HardDisk.bUseHardDiskDirectories)
		{
			core_signal_error("Only one GemDOS hard drive directory can be used.","");
			return;
		}
		param->HardDisk.bFilenameConversion = (ht == 1);
		param->HardDisk.bUseHardDiskDirectories = true;
		strcpy_trunc(param->HardDisk.szHardDiskDirectories[0],image,sizeof(param->HardDisk.szHardDiskDirectories[0]));
		break;
	case 2: // ACSI
		while (i < MAX_ACSI_DEVS && param->Acsi[i].bUseDevice) ++i;
		if (i >= MAX_ACSI_DEVS)
		{
			core_signal_error("Too many ACSI drives, max: ",CORE_STRINGIFY(MAX_ACSI_DEVS));
			return;
		}
		param->Acsi[i].bUseDevice = true;
		strcpy_trunc(param->Acsi[i].sDeviceFile,image,sizeof(param->Acsi[0].sDeviceFile));
		break;
	case 3: // SCSI
		while (i < MAX_SCSI_DEVS && param->Scsi[i].bUseDevice) ++i;
		if (i >= MAX_SCSI_DEVS)
		{
			core_signal_error("Too many SCSI drives, max: ",CORE_STRINGIFY(MAX_SCSI_DEVS));
			return;
		}
		param->Scsi[i].bUseDevice = true;
		strcpy_trunc(param->Scsi[i].sDeviceFile,image,sizeof(param->Scsi[0].sDeviceFile));
		break;
	case 4: // IDE (Auto)
	case 5: // IDE (Byte Swap Off)
	case 6: // IDE (Byte Swap On)
		while (i < MAX_IDE_DEVS && param->Ide[i].bUseDevice) ++i;
		if (i >= MAX_IDE_DEVS)
		{
			core_signal_error("Too many IDE drives, max: ",CORE_STRINGIFY(MAX_IDE_DEVS));
			return;
		}
		param->Ide[i].bUseDevice = true;
		strcpy_trunc(param->Ide[i].sDeviceFile,image,sizeof(param->Ide[0].sDeviceFile));
		{
			static const BYTESWAPPING BSMAP[3] = { BYTESWAP_AUTO, BYTESWAP_OFF, BYTESWAP_ON };
			param->Ide[i].nByteSwap = BSMAP[ht-4];
		}
		break;
	}
}

bool core_config_hard_content(const char* path, int ht)
{
	if (core_hard_content_count >= CORE_HARD_MAX)
	{
		core_signal_error("Too many hard drives, maximum: ",CORE_STRINGIFY(CORE_HARD_MAX));
		return false;
	}

	// GemDOS or IDE can alter their type from the core options menu
	if (ht == 0) // GemDOS 8-bit filenames
	{
		if (ConfigureParams.HardDisk.bFilenameConversion) ht = 1;
	}
	else if (ht == 4) // IDE can be Auto (4), byte swap off (5) or on (6)
	{
		if (ConfigureParams.Ide[0].nByteSwap == BYTESWAP_OFF) ht = 5;
		if (ConfigureParams.Ide[0].nByteSwap == BYTESWAP_ON ) ht = 6;
	}

	core_hard_content = true;
	core_hard_content_type[core_hard_content_count] = ht;
	strcpy_trunc(core_hard_content_path[core_hard_content_count],path,sizeof(core_hard_content_path[0]));
	++core_hard_content_count;
	if (!core_first_reset)
		core_signal_alert2("Hard drive change requires reset: ",path);
	else
		core_config_apply();
	return true;
}

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
	CFG_INT("hatarib_savestate_floppy_modify") core_savestate_floppy_modify = (vi != 0);
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
			core_config_hard(&newparam, image, ht);
		}
	}
	CFG_INT("hatarib_hardboot") newparam.HardDisk.bBootFromHardDisk = vi;
	CFG_INT("hatarib_hard_readonly") { newparam.HardDisk.nWriteProtection = vi; core_hard_readonly = vi; }
	CFG_INT("hatarib_emutos_framerate") newparam.Rom.nEmuTosFramerate = vi;
	CFG_INT("hatarib_emutos_region")
	{
		newparam.Rom.nEmuTosRegion = vi;
		if (newparam.Rom.nEmuTosFramerate < 0 && vi >= 0 && vi < 128) // if default framerate, use EmuTOS country defaults
			newparam.Rom.nEmuTosFramerate = (vi == 0) ? 0 : 1; // NTSC if USA, otherwise PAL
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
	CFG_INT("hatarib_osk_layout") core_osk_layout = vi;
	CFG_INT("hatarib_osk_press_len") core_osk_press_len = vi;
	CFG_INT("hatarib_osk_repeat_delay") core_osk_repeat_delay = vi;
	CFG_INT("hatarib_osk_repeat_rate") core_osk_repeat_rate = vi;
	CFG_INT("hatarib_res2x")
	{
		newparam.Screen.bLowResolutionDouble = (vi >= 2) ? 1 : 0;
		newparam.Screen.bMedResolutionDouble = (vi >= 1) ? 1 : 0;
	}
	CFG_INT("hatarib_borders") { newparam.Screen.bAllowOverscan = (vi != 0); newparam.Screen.nCropOverscan = vi; }
	CFG_INT("hatarib_statusbar") { newparam.Screen.bShowStatusbar = (vi==1); newparam.Screen.bShowDriveLed = (vi==2); }
	CFG_INT("hatarib_aspect") { if (core_video_aspect_mode != vi) { core_video_aspect_mode = vi; core_video_changed = true; } }
	CFG_INT("hatarib_pause_osk") core_pause_osk = vi;
	CFG_INT("hatarib_show_welcome") core_show_welcome = vi;
	CFG_INT("hatarib_boot_alert") core_boot_alert = vi;
	CFG_INT("hatarib_samplerate") newparam.Sound.nPlaybackFreq = vi;
	CFG_INT("hatarib_ymmix") newparam.Sound.YmVolumeMixing = vi;
	CFG_INT("hatarib_lpf") newparam.Sound.YmLpf = vi;
	CFG_INT("hatarib_hpf") newparam.Sound.YmHpf = vi;
	CFG_INT("hatarib_midi") core_midi_enable = (vi != 0);
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
	CFG_INT("hatarib_log_hatari") newparam.Log.nTextLogLevel = vi;
	CFG_INT("hatarib_perf_counters") core_perf_display = (vi != 0);
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
	if (core_hard_content)
	{
		// clear existing drives
		newparam.HardDisk.bUseHardDiskDirectories = false;
		for (int i=0; i<MAX_ACSI_DEVS; ++i) newparam.Acsi[i].bUseDevice = false;
		for (int i=0; i<MAX_SCSI_DEVS; ++i) newparam.Scsi[i].bUseDevice = false;
		for (int i=0; i<MAX_IDE_DEVS; ++i) newparam.Ide[i].bUseDevice = false;

		for (int i=0; i<core_hard_content_count; ++i)
			core_config_hard(&newparam, core_hard_content_path[i], core_hard_content_type[i]);
	}
}

void config_cycle_cpu_speed(void)
{
	int speed = 8;
	const char* name = "8 MHz";
	switch (ConfigureParams.System.nCpuFreq)
	{
	case  8: speed = 16; name = "16 MHz"; break;
	case 16: speed = 32; name = "32 MHz"; break;
	case 32: default: break; // 8 Mhz
	}

	// apply new speed
	Configuration_ChangeCpuFreq(speed);
	ConfigureParams.System.nCpuFreq = speed;
	Statusbar_UpdateInfo();

	// notify
	core_signal_alert2("Cpu Speed: ",name);
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
	Configuration_ChangeCpuFreq(ConfigureParams.System.nBootCpuFreq);
	ConfigureParams.System.nCpuFreq = ConfigureParams.System.nBootCpuFreq;
	Statusbar_UpdateInfo();
}
