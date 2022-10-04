
#include "sprites.h"
#include "window.h"
#include "embedded.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

SDL_Texture* g_sprite_sheet_masks[2] = {NULL, NULL};

bool g_debug_init = false;

void load_sprite_sheet(void)
{
	SDL_RWops* rwops_png = SDL_RWFromConstMem(
		g_asset_sprite_sheet_3_png,
		g_asset_sprite_sheet_3_png_size);
	assert(rwops_png != NULL);
	SDL_Surface* raw_surface = IMG_LoadPNG_RW(rwops_png);
	SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
	SDL_Surface* surface = SDL_ConvertSurface(raw_surface, format, 0);
	SDL_Surface* surface_color_1 = SDL_CreateRGBSurface(0, surface->w, surface->h, )
	SDL_LockSurface(surface);
	struct rgba_t
	{
		uint8_t r, g, b, a;
	};
	typedef struct rgba_t rgba_t;
	rgba_t* pixels = surface->pixels;
	pixels[]
	SDL_UnlockSurface(surface);
	SDL_FreeFormat(format);
	assert(surface != NULL);
	g_sprite_sheet_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(g_sprite_sheet_texture != NULL);
	SDL_FreeSurface(surface);
	if (g_debug_init)
	{
		printf("[init] Loaded sprite sheet.\n");
	}
}

SDL_Rect sprite_src_rect(sprite_t sprite)
{
	switch (sprite)
	{
		case SPRITE_UNIT_CAPE: return (SDL_Rect){0*16, 0*16, 16, 16};
		default:               return (SDL_Rect){0*16, 4*16, 16, 16};
	}
}
