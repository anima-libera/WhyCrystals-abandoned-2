
#include "sprites.h"
#include "rendering.h"
#include "embedded.h"
#include <assert.h>
#include <SDL2/SDL_image.h>

static SDL_Texture* s_sprite_sheet_texture_table[2] = {NULL, NULL};

struct sprite_ref_t
{
	int sheet_index;
	SDL_Rect rect;
};
typedef struct sprite_ref_t sprite_ref_t;

static sprite_ref_t s_sprite_rect_table[] = {
	#define R16(x_, y_, w_, h_) {.x = x_ * 16, .y = y_ * 16, .w = w_ * 16, .h = h_ * 16}
	[SPRITE_ROCK] =                   {1, R16(4, 5, 1, 1)},
	[SPRITE_TREE] =                   {1, R16(1, 4, 1, 2)},
	[SPRITE_CRYSTAL] =                {1, R16(0, 4, 1, 2)},
	[SPRITE_GRASSLAND_DECORATION_0] = {1, R16(4, 0, 1, 1)},
	[SPRITE_GRASSLAND_DECORATION_1] = {1, R16(5, 0, 1, 1)},
	[SPRITE_GRASSLAND_DECORATION_2] = {1, R16(6, 0, 1, 1)},
	[SPRITE_GRASSLAND_DECORATION_3] = {1, R16(7, 0, 1, 1)},
	[SPRITE_UNIT_BASIC] =             {1, R16(4, 1, 1, 1)},
	[SPRITE_UNIT_WALKER] =            {1, R16(5, 1, 1, 1)},
	[SPRITE_UNIT_SHROOM] =            {1, R16(6, 1, 1, 1)},
	[SPRITE_CAN_STILL_ACT] =          {1, {1*16, 3*16, 6, 6}},
	[SPRITE_WALK] =                   {0, R16(2, 4, 1, 1)},
	[SPRITE_TOWER_YELLOW] =           {1, R16(4, 3, 1, 1)},
	[SPRITE_TOWER_YELLOW_OFF] =       {1, R16(4, 4, 1, 1)},
	[SPRITE_TOWER_BLUE] =             {1, R16(6, 3, 1, 1)},
	[SPRITE_TOWER_BLUE_OFF] =         {1, R16(6, 4, 1, 1)},
	[SPRITE_TOWER_GREEN] =            {0, R16(4, 4, 1, 1)},
	[SPRITE_TOWER_WHITE] =            {0, R16(5, 4, 1, 1)},
	[SPRITE_SHOT_BLUE] =              {1, R16(2, 6, 1, 1)},
	[SPRITE_SHOT_RED] =               {1, R16(3, 6, 1, 1)},
	[SPRITE_BLOB_RED] =               {1, R16(4, 6, 1, 1)},
	[SPRITE_ENEMY_EGG] =              {1, R16(7, 2, 1, 1)},
	[SPRITE_ENEMY_BLOB] =             {1, R16(4, 2, 1, 1)},
	[SPRITE_ENEMY_LEGS] =             {1, R16(5, 2, 1, 1)},
	#undef R16
};

void init_sprite_sheet(void)
{
	/* Sprite sheet 1. */
	SDL_RWops* rwops_png = SDL_RWFromConstMem(
		g_asset_sprite_sheet_png,
		g_asset_sprite_sheet_png_size);
	assert(rwops_png != NULL);
	SDL_Surface* surface = IMG_LoadPNG_RW(rwops_png);
	assert(surface != NULL);
	s_sprite_sheet_texture_table[0] = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(s_sprite_sheet_texture_table[0] != NULL);
	SDL_FreeSurface(surface);

	/* Sprite sheet 2. */
	rwops_png = SDL_RWFromConstMem(
		g_asset_sprite_sheet_2_png,
		g_asset_sprite_sheet_2_png_size);
	assert(rwops_png != NULL);
	surface = IMG_LoadPNG_RW(rwops_png);
	assert(surface != NULL);
	s_sprite_sheet_texture_table[1] = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(s_sprite_sheet_texture_table[1] != NULL);
	SDL_FreeSurface(surface);
}

void cleanup_sprite_sheet(void)
{
	SDL_DestroyTexture(s_sprite_sheet_texture_table[0]);
	SDL_DestroyTexture(s_sprite_sheet_texture_table[1]);
}

void draw_sprite(sprite_t sprite, SDL_Rect const* dst_rect)
{
	assert(s_sprite_sheet_texture_table != NULL);
	SDL_RenderCopy(g_renderer,
		s_sprite_sheet_texture_table[s_sprite_rect_table[sprite].sheet_index],
		&s_sprite_rect_table[sprite].rect, dst_rect);
}
