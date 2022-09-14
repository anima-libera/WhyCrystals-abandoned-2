
#ifndef WHYCRYSTALS_HEADER_SPRITES_
#define WHYCRYSTALS_HEADER_SPRITES_

#include <SDL2/SDL.h>

enum sprite_t
{
	SPRITE_ROCK_1,
	SPRITE_ROCK_2,
	SPRITE_ROCK_3,
	SPRITE_TREE,
	SPRITE_CRYSTAL,
	SPRITE_GRASSLAND_0,
	SPRITE_GRASSLAND_1,
	SPRITE_GRASSLAND_2,
	SPRITE_GRASSLAND_3,
	SPRITE_UNIT_BASIC,
	SPRITE_UNIT_WALKER,
	SPRITE_UNIT_SHROOM,
	SPRITE_UNIT_DASH,
	SPRITE_CAN_STILL_ACT,
	SPRITE_WALK,
	SPRITE_TOWER_YELLOW,
	SPRITE_TOWER_YELLOW_OFF,
	SPRITE_TOWER_BLUE,
	SPRITE_TOWER_BLUE_OFF,
	SPRITE_MACHINE_MULTIACT,
	SPRITE_SHOT_BLUE,
	SPRITE_SHOT_RED,
	SPRITE_BLOB_RED,
	SPRITE_ENEMY_EGG,
	SPRITE_ENEMY_SPEEDER,
	SPRITE_ENEMY_BLOB,
	SPRITE_ENEMY_LEGS,
	SPRITE_ENEMY_FAST,
	SPRITE_ENEMY_BIG,
	SPRITE_ARROW_UP,
	SPRITE_ARROW_RIGHT,
	SPRITE_ARROW_DOWN,
	SPRITE_ARROW_LEFT,
	SPRITE_UNKNOWN,
};
typedef enum sprite_t sprite_t;

void init_sprite_sheet(void);
void cleanup_sprite_sheet(void);

void draw_sprite(sprite_t sprite, SDL_Rect const* dst_rect);

#endif /* WHYCRYSTALS_HEADER_SPRITES_ */
