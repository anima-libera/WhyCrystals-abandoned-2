
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
	[SPRITE_ROCK_1] =           {1, R16( 2, 5, 1, 1)},
	[SPRITE_ROCK_2] =           {1, R16( 3, 5, 1, 1)},
	[SPRITE_ROCK_3] =           {1, R16( 4, 5, 1, 1)},
	[SPRITE_TREE] =             {1, R16( 1, 4, 1, 2)},
	[SPRITE_ORANGE_BUSH] =      {1, R16( 2, 8, 1, 1)},
	[SPRITE_CRYSTAL] =          {1, R16( 0, 4, 1, 2)},
	[SPRITE_GRASSLAND_0] =      {1, R16( 4, 0, 1, 1)},
	[SPRITE_GRASSLAND_1] =      {1, R16( 5, 0, 1, 1)},
	[SPRITE_GRASSLAND_2] =      {1, R16( 6, 0, 1, 1)},
	[SPRITE_GRASSLAND_3] =      {1, R16( 7, 0, 1, 1)},
	[SPRITE_UNIT_BASIC] =       {1, R16( 4, 1, 1, 1)},
	[SPRITE_UNIT_WALKER] =      {1, R16( 5, 1, 1, 1)},
	[SPRITE_UNIT_SHROOM] =      {1, R16( 6, 1, 1, 1)},
	[SPRITE_UNIT_DASH] =        {1, R16( 8, 1, 1, 1)},
	[SPRITE_CAN_STILL_ACT] =    {1, {1*16, 3*16, 6, 6}},
	[SPRITE_WALK] =             {1, R16( 2, 3, 1, 1)},
	[SPRITE_TOWER_YELLOW] =     {1, R16( 4, 3, 1, 1)},
	[SPRITE_TOWER_YELLOW_OFF] = {1, R16( 4, 4, 1, 1)},
	[SPRITE_TOWER_BLUE] =       {1, R16( 6, 3, 1, 1)},
	[SPRITE_TOWER_BLUE_OFF] =   {1, R16( 6, 4, 1, 1)},
	[SPRITE_MACHINE_MULTIACT] = {1, R16(11, 3, 1, 1)},
	[SPRITE_SHOT_BLUE] =        {1, R16( 2, 6, 1, 1)},
	[SPRITE_SHOT_RED] =         {1, R16( 3, 6, 1, 1)},
	[SPRITE_BLOB_RED] =         {1, R16( 4, 6, 1, 1)},
	[SPRITE_ENEMY_EGG] =        {1, R16( 7, 2, 1, 1)},
	[SPRITE_ENEMY_SPEEDER] =    {1, R16( 8, 6, 1, 1)},
	[SPRITE_ENEMY_BLOB] =       {1, R16( 4, 2, 1, 1)},
	[SPRITE_ENEMY_LEGS] =       {1, R16( 5, 2, 1, 1)},
	[SPRITE_ENEMY_FAST] =       {1, R16( 6, 2, 1, 1)},
	[SPRITE_ENEMY_BIG] =        {1, R16(12, 2, 1, 1)},
	[SPRITE_ARROW_UP] =         {1, R16( 2, 7, 1, 1)},
	[SPRITE_ARROW_RIGHT] =      {1, R16( 3, 7, 1, 1)},
	[SPRITE_ARROW_DOWN] =       {1, R16( 4, 7, 1, 1)},
	[SPRITE_ARROW_LEFT] =       {1, R16( 5, 7, 1, 1)},
	[SPRITE_UNKNOWN] =          {1, R16( 0, 2, 1, 1)},
	#undef R16
};

void init_sprite_sheet(void)
{
	/* Sprite sheet 2 (the sprite sheet 1 was removed). */
	SDL_RWops* rwops_png = SDL_RWFromConstMem(
		g_asset_sprite_sheet_2_png,
		g_asset_sprite_sheet_2_png_size);
	assert(rwops_png != NULL);
	SDL_Surface* surface = IMG_LoadPNG_RW(rwops_png);
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
	SDL_Texture* texture = s_sprite_sheet_texture_table[s_sprite_rect_table[sprite].sheet_index];
	SDL_RenderCopy(g_renderer, texture, &s_sprite_rect_table[sprite].rect, dst_rect);
}

void draw_sprite_colored(sprite_t sprite, SDL_Rect const* dst_rect, rgb_t color)
{
	assert(s_sprite_sheet_texture_table != NULL);
	SDL_Texture* texture = s_sprite_sheet_texture_table[s_sprite_rect_table[sprite].sheet_index];
	SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
	SDL_RenderCopy(g_renderer, texture, &s_sprite_rect_table[sprite].rect, dst_rect);
	SDL_SetTextureColorMod(texture, 255, 255, 255);
}
