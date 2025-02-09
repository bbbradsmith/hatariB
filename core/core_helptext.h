//
// Help text
// Displayed by the START button by default.
//

// make sure this fits on the 320x200 screen (low res, no doubling, no borders, no statusbar)
const char* const HELPTEXT[] = {
	"hatariB: a Libretro core for Atari ST family emulation,",
	"  an adaptation of Hatari by Brad Smith.",
	"",
	"Default Controls:",
	"  D-Pad or Left-Stick, B, A = Joystick, Fire, Auto Fire",
	"  Right-Stick, Y, X = Mouse, Left Button, Right Button",
	"  L1, R1 = On-Screen Keyboard, One-Shot Keyboard",
	"  L2, R2 = Mouse Slow, Fast",
	"  L3, R3 (Stick-Click) = Space, Return",
	"  Select, Start = Select Floppy Drive, Help",
	"Onscreen Keyboard:",
	"  L1, R1, X = Confirm, Cancel, Toggle Position",
	"Physical Keyboard and Mouse:",
	"  Scroll-Lock = Game Focus to capture Keyboard/Mouse",
	"  F11 = Capture/release Mouse",
	"",
	"Licenses: Hatari (GPLv2), EmuTOS (GPLv2),",
	" Libretro API (MIT), SDL2 (zlib), zlib (zlib)",
	"",
	"https://github.com/bbbradsmith/hatariB/",
	"https://www.hatari-emu.org/",
	SHORTHASH " " __DATE__ " " __TIME__,
};
