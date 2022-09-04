
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
	[SPRITE_ROCK] =                   {0, R16(0, 0, 1, 1)},
	[SPRITE_TREE] =                   {0, R16(1, 6, 1, 1)},
	[SPRITE_CRYSTAL] =                {0, R16(0, 7, 1, 1)},
	[SPRITE_GRASSLAND] =              {0, R16(0, 8, 1, 1)},
	[SPRITE_DESERT] =                 {0, R16(1, 8, 1, 1)},
	[SPRITE_SIDE_DIRT] =              {0, R16(2, 8, 1, 1)},
	[SPRITE_WATER] =                  {0, R16(3, 8, 1, 1)},
	[SPRITE_GRASSLAND_DECORATION_0] = {1, R16(4, 0, 1, 1)},
	[SPRITE_GRASSLAND_DECORATION_1] = {1, R16(5, 0, 1, 1)},
	[SPRITE_GRASSLAND_DECORATION_2] = {1, R16(6, 0, 1, 1)},
	[SPRITE_GRASSLAND_DECORATION_3] = {1, R16(7, 0, 1, 1)},
	[SPRITE_UNIT_RED] =               {0, R16(0, 1, 1, 1)},
	[SPRITE_UNIT_BLUE] =              {0, R16(1, 1, 1, 1)},
	[SPRITE_UNIT_PINK] =              {0, R16(2, 1, 1, 1)},
	[SPRITE_WALK] =                   {0, R16(2, 4, 1, 1)},
	[SPRITE_TOWER_YELLOW] =           {0, R16(0, 4, 1, 1)},
	[SPRITE_TOWER_BLUE] =             {0, R16(3, 4, 1, 1)},
	[SPRITE_TOWER_GREEN] =            {0, R16(4, 4, 1, 1)},
	[SPRITE_TOWER_WHITE] =            {0, R16(5, 4, 1, 1)},
	[SPRITE_TOWER_OFF] =              {0, R16(1, 4, 1, 1)},
	[SPRITE_SHOT_BLUE] =              {0, R16(0, 5, 1, 1)},
	[SPRITE_SHOT_RED] =               {0, R16(2, 5, 1, 1)},
	[SPRITE_BLOB_RED] =               {0, R16(3, 5, 1, 1)},
	[SPRITE_ENEMY_EGG] =              {0, R16(0, 3, 1, 1)},
	[SPRITE_ENEMY_FLY_PURPLE] =       {0, R16(0, 2, 1, 1)},
	[SPRITE_ENEMY_FLY_RED] =          {0, R16(2, 2, 1, 1)},
	[SPRITE_ENEMY_BIG] =              {0, R16(1, 2, 1, 1)},
	[SPRITE_HOVER] =                  {0, R16(0, 9, 1, 1)},
	[SPRITE_SELECT] =                 {0, R16(1, 9, 1, 1)},
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
