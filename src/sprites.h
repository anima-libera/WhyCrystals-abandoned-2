
#ifndef PYEA_HEADER_SPRITES_
#define PYEA_HEADER_SPRITES_

#include <SDL2/SDL.h>

enum sprite_t
{
	SPRITE_ROCK,
	SPRITE_TREE,
	SPRITE_CRYSTAL,
	SPRITE_GRASSLAND,
	SPRITE_DESERT,
	SPRITE_UNIT_RED,
	SPRITE_UNIT_BLUE,
	SPRITE_UNIT_PINK,
	SPRITE_TOWER,
	SPRITE_TOWER_OFF,
	SPRITE_SHOT,
	SPRITE_ENEMY_EGG,
	SPRITE_ENEMY_FLY,
	SPRITE_ENEMY_BIG,
};
typedef enum sprite_t sprite_t;

void init_sprite_sheet(void);
void cleanup_sprite_sheet(void);

void draw_sprite(sprite_t sprite, SDL_Rect const* dst_rect);

#endif /* PYEA_HEADER_SPRITES_ */
