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
int32_t core_osk_press_len = 5;
int core_pause_osk = 2; // help screen default
bool core_osk_begin = false; // true when first entered pause/osk

int32_t core_osk_pos_r;
int32_t core_osk_pos_c;
uint8_t core_osk_pos_display;

void* screen = NULL;
void* screen_copy = NULL;
unsigned int screen_size = 0;
unsigned int screen_copy_size = 0;
unsigned int screen_w = 0;
unsigned int screen_h = 0;
unsigned int screen_p = 0;

#define OSK_ROWS    6
#define OSK_COLS   45

struct OSKey // key information for a 45x6 grid
{
	const char* name; // NULL for an empty space
	int cells; // cell width (normam small key is 2 wide), 0 marks end of row
	int32_t key; // retro_key
	uint8_t mod; // 0 unless this is a toggle-able modifier
};

static int core_osk_layout_set = -1; //use to know when to rebuild keyboard
static const struct OSKey* osk_row[OSK_ROWS];
static int osk_grid[OSK_ROWS][OSK_COLS]; // mappings to osk_row entries, -1 for empty space

//
// Help text
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
	"  L3, R3 (Stick-Click) = Space, Return",
	"  Select, Start = Select Floppy Drive, Help",
	"Onscreen Keyboard:",
	"  L1, R1, X = Confirm, Cancel, Toggle Position",
	"Physical Keyboard and Mouse:",
	"  Scroll-Lock = Game Focus to capture Keyboard/Mouse",
	"  F11 = Capture/release Mouse",
	"",
	"Licenses: Hatari (GPLv2), Libretro API (MIT),",
	"  EmuTOS (GPLv2)",
	"",
	"https://github.com/bbbradsmith/hatariB/",
	"https://hatari.tuxfamily.org/",
};

//
// Keyboard layouts
//
// grid of 45 x 6 keys, a normal small key is 2 cells wide
// at smallest resolution characters are 5px wide, and the 2-cell key takes 14 pixels:
//   2+5+5+1+1 = left edge + character + character + right edge + space
//

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
	{"1!",2,RETROK_1,0},
	{"2@",2,RETROK_2,0},
	{"3#",2,RETROK_3,0},
	{"4$",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6^",2,RETROK_6,0},
	{"7&",2,RETROK_7,0},
	{"8*",2,RETROK_8,0},
	{"9(",2,RETROK_9,0},
	{"0)",2,RETROK_0,0},
	{"-_",2,RETROK_MINUS,0},
	{"=+",2,RETROK_EQUALS,0},
	{"`~",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0}, // (Help)
	{"Und",3,RETROK_END,0}, // (Undo)
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
	{"[{",2,RETROK_LEFTBRACKET,0},
	{"]}",2,RETROK_RIGHTBRACKET,0},
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
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{";:",2,RETROK_SEMICOLON,0},
	{"'\"",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\|",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_QWERTY[] = {
	{"Shift",5,RETROK_LSHIFT,OSK_PRESS_SHL},
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
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW5_QWERTY[] = {
	{0,2,0,0},
	{"Alt",4,RETROK_LALT,OSK_PRESS_ALT},
	{"",18,RETROK_SPACE,0},
	{"Caps",4,RETROK_CAPSLOCK,0},
	{0,9,0,0},
	{"0",4,RETROK_KP0,0},
	{".",2,RETROK_KP_PERIOD,0},
	{"En",2,RETROK_KP_ENTER,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_QWERTY[OSK_ROWS] = {
	OSK_ROW0_QWERTY,
	OSK_ROW1_QWERTY,
	OSK_ROW2_QWERTY,
	OSK_ROW3_QWERTY,
	OSK_ROW4_QWERTY,
	OSK_ROW5_QWERTY,
};

static const struct OSKey* const * const OSK_LAYOUTS[] = {
	OSK_ROWS_QWERTY
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
	// select layout
	for (int r=0; r<OSK_ROWS; ++r)
		osk_row[r] = OSK_LAYOUTS[core_osk_layout][r];
	core_osk_layout_set = core_osk_layout;

	// rebuild grid
	for (int r=0; r<6; ++r)
	{
		int c = 0;
		for (int k=0; (osk_row[r][k].cells > 0) && (c < OSK_COLS); ++k)
		{
			int ki = osk_row[r][k].name ? k : -1; // unnamed keys use -1 for empty space
			for (int i=0; (i < osk_row[r][k].cells) && (c < OSK_COLS); ++i, ++c)
				osk_grid[r][c] = ki;
		}
		for (; c < OSK_COLS; ++c) // fill empty to end of row
			osk_grid[r][c] = -1;
	}
}

extern Uint32 Screen_GetGenConvHeight(void); // hatari/src/screen.c

static void render_keyboard(void)
{
	int gw = sdlgui_fontwidth + 2;
	int gh = sdlgui_fontheight + 5;

	int x0 = (screen_w - (45 * gw)) / 2;
	int y0 = !core_osk_pos_display ? 2 : (Screen_GetGenConvHeight() - ((6 * gh) + 1));

	screen_darken(y0-2,y0+(6*gh+1));
	if (core_osk_layout_set != core_osk_layout) rebuild_keyboard();

	int focus_k = osk_grid[core_osk_pos_r][core_osk_pos_c];
	for (int r=0; r<6; ++r)
	{
		int x = x0;
		int y = y0 + (r * gh);
		bool row_focused = (r == core_osk_pos_r);
		for (int k=0; osk_row[r][k].cells > 0; ++k)
		{
			int w = osk_row[r][k].cells * gw;
			if (osk_row[r][k].name)
			{
				bool focused = row_focused && (k == focus_k);
				bool selected =
					(osk_press_mod & osk_row[r][k].mod) || // modifier toggled
					retrok_down[osk_row[r][k].key]; // show all keys pressed by any means
					//((osk_press_time > 0 ) && (osk_row[r][k].key == osk_press_key)); // or only show the OSK-pressed ones
				SDLGui_DirectBox(x,y,w-1,gh-1,0,focused,selected);
				SDLGui_Text(x+2,y+2,osk_row[r][k].name);
			}
			x += w;
		}
	}
}

static void input_keyboard(uint32_t key)
{
	int c = core_osk_pos_c;
	int r = core_osk_pos_r;

	if (key & AUX_OSK_POS) core_osk_pos_display ^= 1; // flip display position

	if (key & AUX_OSK_CANCEL)
	{
		osk_press_key = 0;
		if (core_osk_mode == CORE_OSK_KEY_SHOT) // allow one-shot cancel to press modifiers
			osk_press_time = core_osk_press_len;
		core_osk_mode = CORE_OSK_OFF;
		return;
	}

	if (key & AUX_OSK_CONFIRM)
	{
		// if it's a mod key, toggle it
		int k = osk_grid[r][c];
		if (k >= 0 && osk_row[r][k].mod)
		{
			osk_press_mod ^= osk_row[r][k].mod;
		}
		else
		{
			// activate the key
			osk_press_key = osk_row[r][k].key;
			osk_press_time = core_osk_press_len;
			if (core_osk_mode == CORE_OSK_KEY_SHOT) // one-shot exist on activation
			{
				core_osk_mode = CORE_OSK_OFF;
				return;
			}
		}
	}

	// exit if no motion
	if (!(key & (AUX_OSK_U | AUX_OSK_D | AUX_OSK_L | AUX_OSK_R))) return;

	// if not on a key, return to the leftmost valid key on grid
	if (osk_grid[r][c] < 0)
	{
		for(c=0; c<OSK_COLS; ++c) if(osk_grid[r][c] > 0) break;
		if (c >= OSK_COLS) c = 0; // oh no, nothing?
	}

	int gk = osk_grid[r][c]; // current key
	// now ready to move
	if (key & AUX_OSK_L)
	{
		// search left until you find a new non-empty key
		int nc = c-1;
		for (; (nc >= 0) && (osk_grid[r][nc] == gk || osk_grid[r][nc] < 0); --nc);
		if (nc >= 0) c = nc;
	}
	if (key & AUX_OSK_R)
	{
		// search right
		int nc = c+1;
		for (; (nc < OSK_COLS) && (osk_grid[r][nc] == gk || osk_grid[r][nc] < 0); ++nc);
		if (nc < OSK_COLS) c = nc;
	}
	if (key & AUX_OSK_U)
	{
		// move to the leftmost cell of key first
		while((c > 0) && (osk_grid[r][c] == osk_grid[r][c-1])) --c;
		// search up for non-empty key
		int nr = r-1;
		for (; (nr >= 0) && (osk_grid[nr][c] < 0); --nr);
		if (nr >= 0)
		{
			r = nr;
		}
		else // retry from rightmost side
		{
			while((c < (OSK_COLS-1)) && (osk_grid[r][c] == osk_grid[r][c+1])) ++c;
			nr = r-1;
			for (; (nr >= 0) && (osk_grid[nr][c] < 0); --nr);
			if (nr >= 0) r = nr;
		}
	}
	if (key & AUX_OSK_D)
	{
		// move to the rightmost cell of key first
		while((c < (OSK_COLS-1)) && (osk_grid[r][c] == osk_grid[r][c+1])) ++c;
		// search down
		int nr = r+1;
		for (; (nr < OSK_ROWS) && (osk_grid[nr][c] < 0); ++nr);
		if (nr < OSK_ROWS)
		{
			r = nr;
		}
		else // retry from leftmost side
		{
			while((c > 0) && (osk_grid[r][c] == osk_grid[r][c-1])) --c;
			nr = r+1;
			for (; (nr < OSK_ROWS) && (osk_grid[nr][c] < 0); ++nr);
			if (nr < OSK_ROWS) r = nr;
		}
	}

	core_osk_pos_c = c;
	core_osk_pos_r = r;
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
	case 2: // Help and Information
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
	case 3: // Floppy Disk List
		{
			static char label[128] = "";
			static const char* HEADER = "Loaded Floppy Disk Images:";
			static const char* NONE = "(NONE)";
			int tw = strlen(HEADER);
			int mw = (screen_w / sdlgui_fontwidth) - 3; // characters that fit on screen
			if (mw > (sizeof(label)-1)) mw = sizeof(label)-1; // character that fit in label
			int n = get_num_images();
			int mh = (screen_h / sdlgui_fontheight) - 3;
			if (n > mh) n = mh; // had intended to leave this uncapped, but SDL's clipping seems to fail mysteriously if I don't?
			// get text width
			for (int i=0; i<n; ++i)
			{
				label[0] = 0;
				get_image_label(i,label,mw);
				int l = strlen(label) + 1; // +1 for left indent
				if (l > tw) tw = l;
			}
			// make box
			tw += 2; // +2 for box sides
			int th = (n < 1) ? 4 : (n + 3); // +3 for HEADER and box top/bottom
			int tx = (screen_w - (sdlgui_fontwidth * tw)) / 2;
			int ty = (screen_h - (sdlgui_fontheight * th)) / 2;
			if (ty < 0) ty = 0; // always keep the top onscreen
			tw *= sdlgui_fontwidth;
			th *= sdlgui_fontheight;
			SDLGui_DirectBox(tx,ty,tw,th,0,false,false);
			// draw lines
			int y = ty + sdlgui_fontheight;
			for (int i=-1; i<n || i<1; ++i)
			{
				int x = tx + sdlgui_fontwidth;
				const char* t = HEADER;
				if (i >= 0)
				{
					x += sdlgui_fontwidth;
					if (i < n)
					{
						label[0] = 0;
						get_image_label(i,label,mw);
						t = label;
					}
					else t = NONE;
				}
				SDLGui_Text(x,y,t);
				y += sdlgui_fontheight;
			}
		}
		break;
	case 4: // Bouncing Box
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
	case 5: // Snow
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
	if (core_osk_begin)
	{
		// don't take inputs on init frame (cancel/confirm could be same as buttons that raise osk)
		// reset the keypresses and modifiers
		osk_press_mod = 0;
		osk_press_key = 0;
		osk_press_time = 0;
		return;
	}
	if (osk_new) input_keyboard(osk_new);
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
	core_serialize_int32(&core_osk_press_len);
	core_serialize_int32(&core_osk_pos_c);
	core_serialize_int32(&core_osk_pos_r);
	core_serialize_uint8(&core_osk_pos_display);
}

void core_osk_init()
{
	core_osk_layout_set = -1; // reinitialize layout
	core_osk_pos_r = 5;
	core_osk_pos_c = 10; // start on space bar?
	core_osk_pos_display = 1; // bottom by default
}
