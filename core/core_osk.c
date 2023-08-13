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

int core_pause_osk = 2; // help screen default
int32_t core_osk_mode = CORE_OSK_OFF;
bool core_osk_init = false; // true when first entered pause/osk

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

static void render_keyboard(void)
{
	// TODO just indicate with darken for now
	screen_darken(screen_h*1/4,screen_h*3/4);
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
			if (bpx < 0 || bpx > pw || core_osk_init)
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
			if (fw != screen_w || fh != screen_h || core_osk_init) // reinitialize
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
	if (core_osk_init) return; // don't take inputs on init frame (cancel/confirm could be same as buttons that raise osk)

	// TODO just be able to cancel keyboard for now
	if (osk_new)
	{
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
	
	core_osk_init = false;
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
	// TODO seralize OSK state
}
