#include <SDL.h>
#include "core.h"

//
// two allowed pixel formats, ARGB, and Indexed with a global palette
//

static Uint32 pixel_palette_key = ~0U;
static Uint32 pixel_palette_map[256] = {};
static SDL_Color pixel_palette_colors[256] = {};
static SDL_Palette pixel_palette = { 256, pixel_palette_colors, 0, 1 };

static SDL_PixelFormat pixelformat_ARGB = {
    SDL_PIXELFORMAT_ARGB8888,
    NULL, // palette
    32, 4, // bits per pixel, bytes per pixel
    {0,0}, // padding
    0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000,
    0, 0, 0, 0, // precision loss (none, full 8-bit)
    16, 8, 0, 24, // shift
    1, // refcount
    NULL // next
};

static SDL_PixelFormat pixelformat_I8 = {
    SDL_PIXELFORMAT_INDEX8,
    &pixel_palette,
    8, 1,
    {0,0},
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    1,
    NULL
};

//
// Rendering functions, used for GUI
//

int SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color)
{
    // ARGB assumed
    if (!dst) return -1;
    int pitch = dst->pitch / 4;
    int x = 0;
    int y = 0;
    int w = dst->w;
    int h = dst->h;
    if (rect)
    {
        x = rect->x;
        y = rect->y;
        w = rect->w;
        h = rect->h;
    }
    if (w < 1 || h < 1) return 0;
    Uint32* p = ((Uint32*)dst->pixels) + (y * pitch) + x;
    for (; h; --h)
    {
        for (int i=0; i<w; ++i)
            p[i] = color;
        p += pitch;
    }
    return 0;
}

// screen.c, sdlgui.c as SDL_BlitSurface
int SDL_UpperBlit(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    if (!dst || !src) return -1;
    int sx = 0;
    int sy = 0;
    int sw = src->w;
    int sh = src->h;
    if (srcrect)
    {
        sx = srcrect->x;
        sy = srcrect->y;
        sw = srcrect->w;
        sh = srcrect->h;
    }
    int dx = 0;
    int dy = 0;
    if (dstrect)
    {
        dx = dstrect->x;
        dy = dstrect->y;
    }
    if (sw < 1 || sh < 1) return 0;

    // dst is always assumed 32-bpp
    int dpitch = dst->pitch / 4;
    Uint32* dp = ((Uint32*)dst->pixels) + (dy * dpitch) + dx;
    if (src->format->BitsPerPixel == 32) // copy
    {
        int spitch = src->pitch / 4;
        Uint32* sp = ((Uint32*)src->pixels) + (sy * spitch) + sx;
        for (; sh; --sh)
        {
            for (int i=0; i<sw; ++i)
                dp[i] = sp[i];
            sp += spitch;
            dp += dpitch;
        }
    }
    else if (src->format->BitsPerPixel == 8) // map indexed with key
    {
        int spitch = src->pitch;
        Uint8* sp = ((Uint8*)src->pixels) + (sy * spitch) + sx;
        for (; sh; --sh)
        {
            for (int i=0; i<sw; ++i)
            {
                if (sp[i] != pixel_palette_key)
                    dp[i] = pixel_palette_map[sp[i]];
            }
            sp += spitch;
            dp += dpitch;
        }
    }
    else return -1;
    return 0;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    (void)Rmask; // ARGB or 8-bit palette assumed
    (void)Gmask;
    (void)Bmask;
    (void)Amask;

    SDL_Surface* surface = (SDL_Surface*)calloc(1,sizeof(SDL_Surface));
    if (surface == NULL) return NULL;

    surface->flags = flags; // ignored
    surface->w = width;
    surface->h = height;  

    if (depth == 32) // ARGB
    {
        surface->format = &pixelformat_ARGB;
        surface->pitch = 4 * width;
    }
    else if(depth == 8) // 8-bit palette
    {
        surface->format = &pixelformat_I8;
        surface->pitch = width;
    }
    else
    {
        core_error_printf("SDL_CreateRGBSurface with unsupported bit depth: %d",depth);
        free(surface);
        return NULL;
    }

    surface->pixels = (void*)malloc(height * surface->pitch);
    if (surface->pixels == NULL)
    {
        free(surface);
        return NULL;
    }

    surface->refcount = 1;
    return surface;
}

void SDL_FreeSurface(SDL_Surface *surface)
{
    if (surface && surface->pixels) free(surface->pixels);
    if (surface) free(surface);
}

int SDL_LockSurface(SDL_Surface *surface)
{
    (void)surface;
    return 0;
}

void SDL_UnlockSurface(SDL_Surface *surface)
{
    (void)surface;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b)
{
    // assume ARGB
    (void)format;
    return 0xFF000000 | (r<<16) | (g<<8) | (b<<0);
}

int SDL_SetColorKey(SDL_Surface *surface, int flag, Uint32 key)
{
    (void)surface;
    (void)flag;
    pixel_palette_key = key;
    return 0;
}

int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors)
{
    for (int i=0; i<ncolors; ++i)
    {
        palette->colors[i+firstcolor] = colors[i];
        pixel_palette_map[i+firstcolor] = SDL_MapRGB(NULL, colors[i].r, colors[i].g, colors[i].b);
    }
    return 0;
}
