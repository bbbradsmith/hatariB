#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include <SDL2/SDL.h>
#include "../hatari/src/includes/sdlgui.h"

// hatari/src/screen.c
extern SDL_Surface* sdlscrn;
// hatari/src/sdl-gui/sdlgui.c
extern SDL_Surface* pSdlGuiScrn;
extern void SDLGui_DirectBox(int x, int y, int w, int h, int offset, bool focused, bool selected);

int32_t core_osk_layout = 0;
int32_t core_osk_mode = CORE_OSK_OFF;
int core_pause_osk = 2; // help screen default
bool core_osk_begin = false; // true when first entered pause/osk

static int core_osk_layout_set = -1; //use to know when to rebuild keyboard
int32_t core_osk_pos_x;
int32_t core_osk_pos_y;
uint8_t core_osk_pos_display;

void* screen = NULL;
void* screen_copy = NULL;
unsigned int screen_size = 0;
unsigned int screen_copy_size = 0;
unsigned int screen_w = 0;
unsigned int screen_h = 0;
unsigned int screen_p = 0;

// make sure this fits on the 320x200 screen (low res, no doubling, no borders, no statusbar)
const char* const HELPTEXT[] = {
	"hatariB: a Libretro core for Atari ST family emulation,",
	"  an adaptation of Hatari by Brad Smith.",
	"",
	"Default Controls:",
	"  D-Pad or Left-Stick, B, A = Joystick, Fire, Auto Fire",
	"  Right-Stick, Y, X = Mouse, Left Button, Right Button",
	"  L1, R1 = On-Screen Keyboard, One-Shot Keyboard",
	"  L3, R3 (Stick-Click) = Space, Return",
	"  Select, Start = Select Floppy Drive, Help",
	"Onscreen Keyboard:",
	"  L1, R1, X = Confirm, Cancel, Toggle Position",
	"Physical Keyboard and Mouse:",
	"  Scroll-Lock = Game Focus to capture Keyboard/Mouse",
	"  F11 = Capture/release Mouse",
	"",
	"Licenses: Hatari (GPLv2), Libretro API (MIT),",
	"  EmuTOS (GPLv2), miniz (MIT)",
	"",
	"https://github.com/bbbradsmith/hatariB/",
	"https://hatari.tuxfamily.org/",
};

//
// Keyboard layouts
//

// modifier keys 
#define OSK_MOD_CTRL   1
#define OSK_MOD_ALT    2
#define OSK_MOD_SHL    3
#define OSK_MOD_SHR    4


// grid of 45 x 6 keys, any visible key is minimum 2 grid points
// at smallest resolution characters are 5px wide, and the 2 grid key takes 14 pixels:
//   2+5+5+1+1 = left edge + character + character + right edge + space
struct OSKey
{
	const char* name; // 2 or 4 characters, NULL for an empty space
	int cells; // 1 for space, 2+ for key, 0 marks end of row
	int key; // retro_key
	int mod; // 0 unless this is a toggle-able modifier
};

const struct OSKey OSK_ROW0_QWERTY[] = {
	{0,1,0,0},
	{"F1",3,RETROK_F1,0},
	{"F2",3,RETROK_F2,0},
	{"F3",3,RETROK_F3,0},
	{"F4",3,RETROK_F4,0},
	{"F5",3,RETROK_F5,0},
	{"F6",3,RETROK_F6,0},
	{"F7",3,RETROK_F7,0},
	{"F8",3,RETROK_F8,0},
	{"F9",3,RETROK_F9,0},
	{"F10",3,RETROK_F10,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW1_QWERTY[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1",2,RETROK_1,0},
	{"2",2,RETROK_2,0},
	{"3",2,RETROK_3,0},
	{"4",2,RETROK_4,0},
	{"5",2,RETROK_5,0},
	{"6",2,RETROK_6,0},
	{"7",2,RETROK_7,0},
	{"8",2,RETROK_8,0},
	{"9",2,RETROK_9,0},
	{"0",2,RETROK_0,0},
	{"-",2,RETROK_MINUS,0},
	{"=",2,RETROK_EQUALS,0},
	{"`~",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_F11,0}, // (Help)
	{"Und",3,RETROK_F12,0}, // (Undo)
	{"(",2,RETROK_PAGEUP,0}, // (Left Paren)
	{")",2,RETROK_PAGEDOWN,0}, // (Right Paren)
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_QWERTY[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Y",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"[",2,RETROK_LEFTBRACKET,0},
	{"]",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_QWERTY[] = {
	{"Ctrl",4,RETROK_LCTRL,0},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{";",2,RETROK_SEMICOLON,0},
	{"'\"",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_QWERTY[] = {
	{"Shift",5,RETROK_LSHIFT,0},
	{"Z",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{"M",2,RETROK_m,0},
	{",<",2,RETROK_COMMA,0},
	{".>",2,RETROK_PERIOD,0},
	{"/?",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,0},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW5_QWERTY[] = {
	{0,2,0,0},
	{"Alt",4,RETROK_LALT,0},
	{"",18,RETROK_z,0},
	{"Caps",4,RETROK_x,0},
	{0,9,0,0},
	{"0",4,RETROK_KP0,0},
	{".",2,RETROK_KP_PERIOD,0},
	{"En",2,RETROK_KP_ENTER,0},
	{0,0,0,0}};

static const struct OSKey* osk_row[6] = {
	OSK_ROW0_QWERTY,
	OSK_ROW1_QWERTY,
	OSK_ROW2_QWERTY,
	OSK_ROW3_QWERTY,
	OSK_ROW4_QWERTY,
	OSK_ROW5_QWERTY,
};

//
// 50% efficient screen darkening by bit shifting and masking
//   each colour is contiguous bits,
//   and >> 1 will penetrate at most the top bit of another colour,
//   but we can mask the penetrated bits away
//

uint32_t darken_mask = 0;
uint32_t darken_alpha = 0;

// should only need to be called once, because format will never change
// (e.g. call if darken_mask=0)
static void set_darken_mask(SDL_Surface* surf)
{
	if (!surf) return;
	SDL_PixelFormat* fmt = surf->format;
	if (!fmt) return;

	darken_mask =
		((fmt->Rmask >> 1) & (fmt->Rmask)) |
		((fmt->Gmask >> 1) & (fmt->Gmask)) |
		((fmt->Bmask >> 1) & (fmt->Bmask));
	darken_alpha = fmt->Amask;

	// double to fill 32 bits
	if (fmt->BytesPerPixel <= 2)
	{
		darken_mask |= (darken_mask << 16);
		darken_alpha |= (darken_alpha << 16);
	}
}

static void screen_darken(unsigned int y0, unsigned int y1) // 50% darken from line = y0 up to y1
{
	if (darken_mask == 0) set_darken_mask(sdlscrn);

	uint32_t* row = ((uint32_t*)screen) + ((y0 * screen_p) / 4);
	unsigned int count = ((y1 - y0) * screen_p) / 4;
	if ((count > 0) && (screen_p % 4)) --count; // screen_p should be divisible by 4, but just in case drop a pixel for buffer safety

	for (; count > 0; --count)
	{
		*row = ((*row >> 1) & darken_mask) | darken_alpha;
		++row;
	}
}

static inline void draw_box(int x, int y, int w, int h, Uint32 c)
{
	SDL_Rect r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	SDL_FillRect(sdlscrn, &r, c);
}

//
// on-screen keyboard
//

static void rebuild_keyboard(void)
{
	// TODO reinit osk_row, built the mapping grid, etc.
	core_osk_layout_set = core_osk_layout;
}

extern Uint32 Screen_GetGenConvHeight(void);

static void render_keyboard(void)
{
	int gw = sdlgui_fontwidth + 2;
	int gh = sdlgui_fontheight + 5;

	int x0 = (screen_w - (45 * gw)) / 2;
	int y0 = !core_osk_pos_display ? 2 : (Screen_GetGenConvHeight() - ((6 * gh) + 1));

	screen_darken(y0-2,y0+(6*gh+1));
	if (core_osk_layout_set != core_osk_layout) rebuild_keyboard();
	for (int r=0; r<6; ++r)
	{
		int x = x0;
		int y = y0 + (r * (sdlgui_fontheight+5));
		for(const struct OSKey* k = osk_row[r]; k->cells > 0; ++k)
		{
			int w = k->cells * (sdlgui_fontwidth + 2);
			int h = 5 + sdlgui_fontheight;
			if (k->name)
			{
				//retro_log(RETRO_LOG_DEBUG,"osk: '%s',%d,%d,%d,%d\n",k->name,x,y,w,h);
				// TODO focused/selected flags
				SDLGui_DirectBox(x,y,w-1,h-1,0,false,false);
				SDLGui_Text(x+2,y+2,k->name);
			}
			x += w;
		}
	}
}

//
// pause menu
//

static void render_pause(void)
{

	// No Indicator does not darken the screen, but the rest do
	if (core_pause_osk != 1)
		screen_darken(0,screen_h);

	switch (core_pause_osk)
	{
	default:
	case 0: // Darken
	case 1: // No Indicator (not possible)
		break;
	case 2: // Help
		{
			static int tw = -1;
			static const int th = CORE_ARRAY_SIZE(HELPTEXT);
			// get text width
			if (tw < 0)
			{
				for (int i=0; i<th; ++i)
				{
					int w = strlen(HELPTEXT[i]);
					if (w > tw) tw = w;
				}
			}
			int tx = (screen_w - (sdlgui_fontwidth * tw)) / 2;
			int ty = (screen_h - (sdlgui_fontheight * th)) / 2;
			SDLGui_DirectBox(
				tx-sdlgui_fontwidth,
				ty-sdlgui_fontheight,
				((tw+2)*sdlgui_fontwidth),
				((th+2)*sdlgui_fontheight),
				0, false, false);
			for (int i=0; i<th; ++i)
				SDLGui_Text(tx,ty+(sdlgui_fontheight*i),HELPTEXT[i]);
		}
		break;
	case 3: // Bouncing Box
		{
			static int bpx = -1;
			static int bpy = -1;
			static int dx = 1;
			static int dy = 1;
			const int bw = sdlgui_fontwidth * 9;
			const int bh = sdlgui_fontheight * 3;
			const int pw = screen_w - bw;
			const int ph = screen_h - bh;
			if (bpx < 0 || bpx > pw || core_osk_begin)
			{
				bpx = rand() % (pw-2) + 1;
				dx = (rand() & 2) - 1;
			}
			if (bpy < 0 || bpy > ph)
			{
				bpy = rand() % (ph-2) + 1;
				dy = (rand() & 2) - 1;
			}
			bpx += dx;
			bpy += dy;
			if (bpx <= 0) dx = 1;
			else if (bpx >= pw) dx = -1;
			if (bpy <= 0) dy = 1;
			if (bpy >= ph) dy = -1;
			SDLGui_DirectBox(bpx,bpy,bw,bh,0,false,false);
			SDLGui_Text(bpx+sdlgui_fontwidth,bpy+sdlgui_fontheight,"hatariB");
		}
		break;
	case 4: // Snow
		{
			// Inspired by the ZSNES snow
			// and by Downfall from ST Format 42
			#define SNOWFLAKES   128
			static int fw = 0;
			static int fh = 0;
			static float fwf;
			static float fhf;
			static float flakex[SNOWFLAKES];
			static float flakey[SNOWFLAKES];
			static float flakedx[SNOWFLAKES];
			static float flakedy[SNOWFLAKES];
			static float sf;
			static int si;
			static Uint32 white = 0;
			if (fw != screen_w || fh != screen_h || core_osk_begin) // reinitialize
			{
				white = SDL_MapRGB(sdlscrn->format,255,255,255);
				fw = screen_w;
				fh = screen_h;
				si = fw / 320;
				fwf = (float)fw;
				fhf = (float)fh;
				sf = (float)si;
				for (int i=0; i<SNOWFLAKES; ++i)
				{
					flakey[i] = -(rand() % fh);
					flakedy[i] = -1; // marks uninitialized
				}
			}
			for (int i=0; i<SNOWFLAKES; ++i)
			{
				if (flakey[i] >= fhf || flakedy[i] < 0)
				{
					flakex[i] = rand() % fw;
					if (flakey[i] >= fhf) flakey[i] = 0;
					flakedx[i] = (sf / 256.0) * (float)((rand() % 256) - 128);
					flakedy[i] = (sf / 256.0) * (float)((rand() % 128) + 128);
				}
				flakex[i] += flakedx[i];
				if (flakex[i] >= fwf) flakex[i] -= (fwf+sf);
				if (flakex[i] <  -sf) flakex[i] += (fwf+sf);
				flakey[i] += flakedy[i];
				draw_box((int)flakex[i],(int)flakey[i],si,si,white);
			}
		}
		break;
	}
	return;
}

//
// public interface
//

void core_osk_input(uint32_t osk_new)
{
	if (core_osk_mode < CORE_OSK_KEY) return; // pause menu does not take input
	if (core_osk_begin) return; // don't take inputs on init frame (cancel/confirm could be same as buttons that raise osk)

	// TODO just be able to cancel keyboard for now
	if (osk_new)
	{
		// make sure keyboard layout matches before doing anything
		if (core_osk_layout_set != core_osk_layout) rebuild_keyboard();

		// toggle display position
		if (osk_new & AUX_OSK_POS) core_osk_pos_display ^= 1;

		core_debug_hex("osk_new: ",osk_new);
		if (osk_new & AUX_OSK_CANCEL) core_osk_mode = CORE_OSK_OFF;
	}
}

void core_osk_render(void* video_buffer, int w, int h, int pitch)
{
	if (core_osk_mode == CORE_OSK_OFF || core_osk_mode > CORE_OSK_KEY_SHOT)
	{
		retro_log(RETRO_LOG_ERROR,"Unexpected core_osk_render mode? %d\n",core_osk_mode);
		core_osk_mode = CORE_OSK_OFF;
		return;
	}

	screen = video_buffer;
	screen_size = (unsigned int)(h * pitch);
	if (screen == NULL)
	{
		retro_log(RETRO_LOG_WARN,"No video_buffer, unable to render on-screen overlay.\n");
		screen_size = 0;
		return;
	}
	screen_w = (unsigned int)w;
	screen_h = (unsigned int)h;
	screen_p = (unsigned int)pitch;

	// reallocate screen_copy if needed
	if (screen_copy == NULL || screen_copy_size < screen_size)
	{
		free(screen_copy);
		screen_copy_size = 0;
		screen_copy = malloc(screen_size);
		if (screen_copy) screen_copy_size = screen_size;
		else retro_log(RETRO_LOG_WARN,"Unable to allocate screen_copy for on-screen overlay.\n");
	}

	// save a copy
	if (screen_copy)
		memcpy(screen_copy, screen, screen_size);
	
	if (sdlscrn == NULL)
	{
		retro_log(RETRO_LOG_WARN,"No hatari sdlscrn surface, unable to render on-screen overlay.\n");
		return;
	}

	if (pSdlGuiScrn != sdlscrn) // in case nothing has set it yet
		SDLGui_SetScreen(sdlscrn);

	if (core_osk_mode >= CORE_OSK_KEY)
		render_keyboard();
	else
		render_pause();
	
	core_osk_begin = false;
}

void core_osk_restore(void* video_buffer, int w, int h, int pitch)
{
	// don't restore if screen has changed
	if (screen != video_buffer || screen_w != w || screen_h != h || screen_p != pitch)
		return;

	if (screen && screen_copy)
		memcpy(screen, screen_copy, screen_size);
}

void core_osk_serialize(void)
{
	// pause screen static state doesn't matter, no impact on emulation
	// but the on-screen keyboard state does, and needs its status restored
	core_serialize_int32(&core_osk_mode);
	core_serialize_int32(&core_osk_layout);
	core_serialize_int32(&core_osk_pos_x);
	core_serialize_int32(&core_osk_pos_y);
	core_serialize_uint8(&core_osk_pos_display);
	// TODO mods
}

void core_osk_init()
{
	core_osk_layout_set = -1; // reinitialize layout
	core_osk_pos_x = 10; // start on space bar?
	core_osk_pos_y = 5;
	core_osk_pos_display = 0; // top
}

