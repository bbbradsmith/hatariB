#include "../libretro/libretro.h"
#include "core.h"
#include "core_internal.h"
#include <SDL2/SDL.h>

extern SDL_Surface* sdlscrn; // from hatari/src/screen.c

int core_pause_osk = 2; // help screen default
int core_osk_mode = CORE_OSK_OFF;

void* screen = NULL;
void* screen_copy = NULL;
unsigned int screen_size = 0;
unsigned int screen_copy_size = 0;
unsigned int screen_w = 0;
unsigned int screen_h = 0;
unsigned int screen_p = 0;

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
	uint32_t* row = ((uint32_t*)screen) + ((y0 * screen_p) / 4);
	unsigned int count = ((y1 - y0) * screen_p) / 4;
	for (; count > 0; --count)
	{
		*row = ((*row >> 1) & darken_mask) | darken_alpha;
		++row;
	}
}

void core_osk_input(uint32_t osk_new)
{
	// TODO cancel keyboard for now
	if (core_osk_mode >= CORE_OSK_KEY) core_osk_mode = CORE_OSK_OFF;
}

void core_osk_render(void* video_buffer, int w, int h, int pitch)
{
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

	// TODO darken pause area
	if (!darken_mask) set_darken_mask(sdlscrn);
	screen_darken(h/4,h/2); // test of darkening

	if (sdlscrn == NULL)
	{
		retro_log(RETRO_LOG_WARN,"No hatari sdlscrn surface, unable to render on-screen overlay.\n");
		return;
	}
	// TODO use hatari SDL gui renderer for overlay components
}

void core_osk_restore()
{
	if (screen && screen_copy)
		memcpy(screen, screen_copy, screen_size);
}
