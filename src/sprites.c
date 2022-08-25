
#include "sprites.h"
#include "rendering.h"
#include "embedded.h"
#include <assert.h>
#include <SDL2/SDL_image.h>

static SDL_Texture* s_sprite_sheet_texture = NULL;

static SDL_Rect s_sprite_rect_table[] = {
	#define XY16(x_, y_) .x = x_ * 16, .y = y_ * 16
	#define WH16 .w = 16, .h = 16
	[SPRITE_ROCK] =      {XY16(0, 0), WH16},
	[SPRITE_TREE] =      {XY16(0, 6), WH16},
	[SPRITE_CRYSTAL] =   {XY16(0, 7), WH16},
	[SPRITE_GRASSLAND] = {XY16(0, 8), WH16},
	[SPRITE_DESERT] =    {XY16(1, 8), WH16},
	[SPRITE_UNIT_RED] =  {XY16(0, 1), WH16},
	[SPRITE_UNIT_BLUE] = {XY16(1, 1), WH16},
	[SPRITE_UNIT_PINK] = {XY16(2, 1), WH16},
	[SPRITE_TOWER] =     {XY16(0, 4), WH16},
	[SPRITE_TOWER_OFF] = {XY16(1, 4), WH16},
	[SPRITE_SHOT] =      {XY16(0, 5), WH16},
	[SPRITE_ENEMY_EGG] = {XY16(0, 3), WH16},
	[SPRITE_ENEMY_FLY] = {XY16(0, 2), WH16},
	[SPRITE_ENEMY_BIG] = {XY16(1, 2), WH16},
	[SPRITE_HOVER] =     {XY16(0, 9), WH16},
	[SPRITE_SELECT] =    {XY16(1, 9), WH16},
	#undef XY16
	#undef WH16
};

void init_sprite_sheet(void)
{
	SDL_RWops* rwops_png = SDL_RWFromConstMem(
		g_asset_sprite_sheet_png,
		g_asset_sprite_sheet_png_size);
	assert(rwops_png != NULL);
	SDL_Surface* surface = IMG_LoadPNG_RW(rwops_png);
	assert(surface != NULL);
	s_sprite_sheet_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(s_sprite_sheet_texture != NULL);
	SDL_FreeSurface(surface);
}

void cleanup_sprite_sheet(void)
{
	SDL_DestroyTexture(s_sprite_sheet_texture);
}

void draw_sprite(sprite_t sprite, SDL_Rect const* dst_rect)
{
	assert(s_sprite_sheet_texture != NULL);
	SDL_RenderCopy(g_renderer, s_sprite_sheet_texture,
		&s_sprite_rect_table[sprite], dst_rect);
}
