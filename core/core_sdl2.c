#include <SDL.h>
#include <string.h>

//
// This file is a substite for the very small subset of SDL2 that hatariB depends on.
//

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags,
                     int width, int height, int depth,
                     Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	// TODO
	(void)flags;
	(void)width;
	(void)height;
	(void)depth;
	(void)Rmask;
	(void)Gmask;
	(void)Bmask;
	(void)Amask;
	return NULL;
}

void SDL_FreeSurface(SDL_Surface *surface)
{
	// TODO
	(void)surface;
}

int SDL_LockSurface(SDL_Surface *surface)
{
	// not needed
	(void)surface;
	return 0;
}

void SDL_UnlockSurface(SDL_Surface *surface)
{
	// not needed
	(void)surface;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b)
{
	// TODO
	(void)format;
	(void)r;
	(void)g;
	(void)b;
	return 0;
}

int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors,
                         int firstcolor, int ncolors)
{
	// TODO
	(void)palette;
	(void)colors;
	(void)firstcolor;
	(void)ncolors;
	return 0;
}

int SDL_SetColorKey(SDL_Surface *surface, int flag, Uint32 key)
{
	// TODO
	(void)surface;
	(void)flag;
	(void)key;
	return 0;
}

int SDL_UpperBlit(SDL_Surface *src, const SDL_Rect *srcrect,
                  SDL_Surface *dst, SDL_Rect *dstrect)
{
	// TODO
	(void)src;
	(void)srcrect;
	(void)dst;
	(void)dstrect;
	return 0;
}

int SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color)
{
	// TODO
	(void)dst;
	(void)rect;
	(void)color;
	return 0;
}

// SDL/src/stdlib/SDL_string.c
size_t
SDL_strlcpy(SDL_OUT_Z_CAP(maxlen) char *dst, const char *src, size_t maxlen)
{
	size_t srclen = strlen(src);
	if (maxlen > 0) {
		size_t len = SDL_min(srclen, maxlen - 1);
		memcpy(dst, src, len);
		dst[len] = '\0';
	}
	return srclen;
}
