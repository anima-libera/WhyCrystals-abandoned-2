
#include "window.h"
#include "embedded.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

TTF_Font* g_font;

void load_font(void)
{
	SDL_RWops* rwops_font = SDL_RWFromConstMem(
		g_asset_font,
		g_asset_font_size);
	assert(rwops_font != NULL);
	g_font = TTF_OpenFontRW(rwops_font, 0, 20);
	assert(g_font != NULL);
}
