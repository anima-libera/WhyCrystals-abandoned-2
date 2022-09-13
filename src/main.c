
#include "embedded.h"
#include "rendering.h"
#include "sprites.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <strings.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

bool g_debug;

int max(int a, int b)
{
	return a < b ? b : a;
}

int min(int a, int b)
{
	return a < b ? a : b;
}

/* Changes the length of a dynamic vector.
 * Here is an example of the intended use:
 *    int da_len, da_cap;
 *    elem_t* da;
 *    ...
 *    DA_LENGTHEN(da_len += 1, da_cap, da, elem_t);
 * Notice how the first argument must be an expression that evaluates
 * to the new length of the dynamic array (and if it has the side effect
 * of updating the length variable it is even better).
 * The new elements are not initialized. */
#define DA_LENGTHEN(len_expr_, cap_, array_ptr_, elem_type_) \
	do { \
		int const len_ = len_expr_; \
		if (len_ > cap_) \
		{ \
			int const new_cap = max(len_, ((float)cap_ + 2.3f) * 1.3f); \
			elem_type_* new_array = realloc(array_ptr_, new_cap * sizeof(elem_type_)); \
			assert(new_array != NULL); \
			array_ptr_ = new_array; \
			cap_ = new_cap; \
		} \
	} while (0)

/* Tile coordinates. */
struct tc_t
{
	int x, y;
};
typedef struct tc_t tc_t;

struct rect_t
{
	int x, y, w, h;
};
typedef struct rect_t rect_t;

struct tc_iter_rect_t
{
	tc_t tc;
	rect_t rect;
};
typedef struct tc_iter_rect_t tc_iter_rect_t;

tc_iter_rect_t tc_iter_rect_init(rect_t rect)
{
	return (tc_iter_rect_t){
		.tc = {.x = rect.x, .y = rect.y},
		.rect = rect};
}

rect_t tctc_to_rect(tc_t a, tc_t b)
{
	return (rect_t){
		.x = min(a.x, b.x),
		.y = min(a.y, b.y),
		.w = abs(a.x - b.x) + 1,
		.h = abs(a.y - b.y) + 1};
}

rect_t tc_radius_to_rect(tc_t tc, int radius)
{
	return (rect_t){
		.x = tc.x - radius,
		.y = tc.y - radius,
		.w = radius * 2 + 1,
		.h = radius * 2 + 1};
}

void tc_iter_rect_next(tc_iter_rect_t* it)
{
	it->tc.x++;
	if (it->tc.x > it->rect.x + it->rect.w)
	{
		it->tc.x = it->rect.x;
		it->tc.y++;
	}
}

bool tc_iter_rect_cond(tc_iter_rect_t* it)
{
	return it->tc.y < it->rect.y + it->rect.h;
}

typedef struct tc_iter_all_t tc_iter_all_t;
tc_iter_all_t tc_iter_all_init(void);
void tc_iter_all_next(tc_iter_all_t* it);
bool tc_iter_all_cond(tc_iter_all_t* it);

bool tc_in_map(tc_t tc);

bool tc_eq(tc_t tc_a, tc_t tc_b)
{
	return tc_a.x == tc_b.x && tc_a.y == tc_b.y;
}

tc_t g_crystal_tc;

/* Iterates over the tiles of the map in a spiral pattern
 * centered on the given center and growing outwards. */
struct tc_iter_all_spiral_t
{
	tc_t tc;
	tc_t center;
	bool is_done;
};
typedef struct tc_iter_all_spiral_t tc_iter_all_spiral_t;

tc_iter_all_spiral_t tc_iter_all_spiral_init(tc_t center)
{
	return (tc_iter_all_spiral_t){
		.tc = center,
		.center = center,
		.is_done = false};
}

void tc_iter_all_spiral_next(tc_iter_all_spiral_t* it)
{
	/* In there, the `spiral_tc` will follow the spiral path.
	 * That path is a succession of 'rings' in a <> shape,
	 * from small rings to big rings.
	 * When `spiral_tc` encounters `it.tc` then the next tile is
	 * the good one (this is indicated by `sync` being true),
	 * except if it is outside the map (in which case
	 * we just keep going until we get back in the map).
	 * We actually start by the ring in which `it.tc` already is.
	 * If an entire ring is iterated over without ever touching the map,
	 * then it means it is time to stop. */

	int dist = abs(it->tc.x - it->center.x) + abs(it->tc.y - it->center.y);
	bool sync = tc_eq(it->center, it->tc);
	while (true)
	{
		tc_t spiral_tc = dist == 0 ? it->center :
			(tc_t){.x = it->center.x - dist + 1, .y = it->center.y - 1};
		tc_t ring_end_tc = dist == 0 ? it->center :
			(tc_t){.x = it->center.x - dist, .y = it->center.y};
		bool ring_is_out = true;

		struct dir_t
		{
			int dx, dy;
			int length;
		};
		typedef struct dir_t dir_t;
		dir_t dirs[] = {
			{.dx = 1,  .dy = -1, .length = dist - 1},
			{.dx = 1,  .dy = 1,  .length = dist},
			{.dx = -1, .dy = 1,  .length = dist},
			{.dx = -1, .dy = -1, .length = dist + 1}};
		for (int i = 0; i < 4; i++)
		{
			for (int j = dist - dirs[i].length; j < dist || dist == 0; j++)
			{
				if (tc_in_map(spiral_tc))
				{
					ring_is_out = false;
				}
				if (tc_eq(spiral_tc, it->tc))
				{
					sync = true;
				}

				if (tc_eq(spiral_tc, ring_end_tc))
				{
					/* Going to the next ring. */
					spiral_tc = (tc_t){.x = it->center.x - dist, .y = it->center.y - 1};
				}
				else
				{
					spiral_tc.x += dirs[i].dx;
					spiral_tc.y += dirs[i].dy;
				}

				if (sync && tc_in_map(spiral_tc))
				{
					it->tc = spiral_tc;
					return;
				}
				if (dist == 0)
				{
					/* Special case: The ring at distance zero is just one tile
					 * and the ring ends immediately. */
					goto end_ring;
				}
			}
		}

		end_ring:
		if (ring_is_out)
		{
			/* If the ring never enters the map, then it means all the tiles
			 * of the map have been iterated over, and it has to end. */
			it->is_done = true;
			return;
		}
		dist++;
	}
}

bool tc_iter_all_spiral_cond(tc_iter_all_spiral_t* it)
{
	return !it->is_done;
}

/* Screen coordinates. */
struct sc_t
{
	int x, y;
};
typedef struct sc_t sc_t;

int g_tile_side_pixels = 64;
int g_window_w = 800 - 800 % 64, g_window_h = 800 - 800 % 64;

/* What are the screen coordinates of top left corner of the tile at (0, 0) ? */
sc_t g_tc_sc_offset = {0};

float g_tc_sc_offset_frac_x = 0.0f, g_tc_sc_offset_frac_y = 0.0f;

sc_t tc_to_sc(tc_t tc)
{
	return (sc_t){
		tc.x * g_tile_side_pixels + g_tc_sc_offset.x,
		tc.y * g_tile_side_pixels + g_tc_sc_offset.y};
}

sc_t tc_to_sc_center(tc_t tc)
{
	return (sc_t){
		tc.x * g_tile_side_pixels + g_tc_sc_offset.x + g_tile_side_pixels / 2,
		tc.y * g_tile_side_pixels + g_tc_sc_offset.y + g_tile_side_pixels / 2};
}

tc_t sc_to_tc(sc_t sc)
{
	tc_t tc = {
		(sc.x - g_tc_sc_offset.x) / g_tile_side_pixels,
		(sc.y - g_tc_sc_offset.y) / g_tile_side_pixels};
	if ((sc.x - g_tc_sc_offset.x) < 0)
	{
		tc.x--;
	}
	if ((sc.y - g_tc_sc_offset.y) < 0)
	{
		tc.y--;
	}
	return tc;
}

struct rgb_t
{
	unsigned char r, g, b;
};
typedef struct rgb_t rgb_t;

void draw_text_ex(char const* text, rgb_t color,
	sc_t sc, bool center_x, bool center_y,
	bool bg, rgb_t bg_color)
{
	SDL_Color sdl_color = {.r = color.r, .g = color.g, .b = color.b, .a = 255};
	SDL_Surface* surface = TTF_RenderText_Solid(g_font, text, sdl_color);
	assert(surface != NULL);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(texture != NULL);
	SDL_FreeSurface(surface);
	SDL_Rect rect = {.x = sc.x, .y = sc.y};
	SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
	if (center_x)
	{
		rect.x -= rect.w / 2;
	}
	if (center_y)
	{
		rect.y -= rect.h / 2;
	}
	if (bg)
	{
		SDL_Rect visual_rect = rect;
		visual_rect.y += 4;
		visual_rect.w += 1;
		visual_rect.h -= 8;
		SDL_SetRenderDrawColor(g_renderer, bg_color.r, bg_color.g, bg_color.b, 255);
		SDL_RenderFillRect(g_renderer, &visual_rect);
	}
	SDL_RenderCopy(g_renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
}

void draw_text(char const* text, rgb_t color, sc_t sc)
{
	draw_text_ex(text, color, sc, false, false, false, (rgb_t){0});
}

enum obj_type_t
{
	OBJ_NONE = 0,
	OBJ_ROCK,
	OBJ_TREE,
	OBJ_CRYSTAL,
	OBJ_UNIT_BASIC,
	OBJ_UNIT_WALKER,
	OBJ_UNIT_SHROOM,
	OBJ_UNIT_DASH,
	/* Shoots blue shots but not on an adjacent tile. */
	OBJ_TOWER_YELLOW,
	/* Shoots red shots. */
	OBJ_TOWER_BLUE,
	/* Makes anything on an adjacent tile be able to act
	 * as many times as it can and wants. */
	OBJ_MACHINE_MULTIACT,
	OBJ_SHOT_BLUE,
	/* Spills red blob on impact. */
	OBJ_SHOT_RED,
	OBJ_BLOB_RED,
	OBJ_ENEMY_EGG,
	OBJ_ENEMY_BLOB,
	OBJ_ENEMY_LEGS,

	OBJ_NUMBER
};
typedef enum obj_type_t obj_type_t;

struct obj_t
{
	obj_type_t type;
	bool can_still_act;
	int ammo; /* Used by towers. */
	int life; /* Used by big enemies. */
	int sprite_variant;
};
typedef struct obj_t obj_t;

bool obj_type_is_tower(obj_type_t type);
int g_time;

void draw_obj(obj_t const* obj, sc_t sc, int side)
{
	/* Draw the object sprite. */
	sprite_t sprite;
	bool tall_sprite = false;
	switch (obj->type)
	{
		case OBJ_NONE:             return;
		case OBJ_ROCK:             sprite = SPRITE_ROCK_1 + obj->sprite_variant;          break;
		case OBJ_TREE:             sprite = SPRITE_TREE;             tall_sprite = true; break;
		case OBJ_CRYSTAL:          sprite = SPRITE_CRYSTAL;          tall_sprite = true; break;
		case OBJ_UNIT_WALKER:      sprite = SPRITE_UNIT_WALKER;      break;
		case OBJ_UNIT_BASIC:       sprite = SPRITE_UNIT_BASIC;       break;
		case OBJ_UNIT_SHROOM:      sprite = SPRITE_UNIT_SHROOM;      break;
		case OBJ_UNIT_DASH:        sprite = SPRITE_UNIT_DASH;        break;
		case OBJ_TOWER_YELLOW:     sprite = SPRITE_TOWER_YELLOW;     break;
		case OBJ_TOWER_BLUE:       sprite = SPRITE_TOWER_BLUE;       break;
		case OBJ_MACHINE_MULTIACT: sprite = SPRITE_MACHINE_MULTIACT; break;
		case OBJ_SHOT_BLUE:        sprite = SPRITE_SHOT_BLUE;        break;
		case OBJ_SHOT_RED:         sprite = SPRITE_SHOT_RED;         break;
		case OBJ_BLOB_RED:         sprite = SPRITE_BLOB_RED;         break;
		case OBJ_ENEMY_EGG:        sprite = SPRITE_ENEMY_EGG;        break;
		case OBJ_ENEMY_BLOB:       sprite = SPRITE_ENEMY_BLOB;       break;
		case OBJ_ENEMY_LEGS:       sprite = SPRITE_ENEMY_LEGS;       break;
		default: assert(false);
	}
	if (obj_type_is_tower(obj->type) && obj->ammo <= 0)
	{
		/* Normally a sprite value for a tower is followed by the sprite value
		 * for that same tower but turned off.
		 * For example `SPRITE_TOWER_YELLOW + 1 == SPRITE_TOWER_YELLOW_OFF`. */
		sprite++;
	}
	SDL_Rect tile_rect = {.x = sc.x, .y = sc.y, .w = side, .h = side};
	SDL_Rect obj_rect = tile_rect;
	obj_rect.y -= 2 * (g_tile_side_pixels / 16);
	if (tall_sprite)
	{
		obj_rect.y -= g_tile_side_pixels;
		obj_rect.h *= 2;
	}
	draw_sprite(sprite, &obj_rect);

	/* Draw the can-still-act indicator. */
	if (obj->can_still_act)
	{
		SDL_Rect token_rect = obj_rect;
		token_rect.w = 6 * (g_tile_side_pixels / 16);
		token_rect.h = 6 * (g_tile_side_pixels / 16);
		token_rect.x += g_tile_side_pixels / 2 - token_rect.w / 2;
		token_rect.y -= token_rect.h + 2.0f * cos((float)g_time * 0.03f);
		draw_sprite(SPRITE_CAN_STILL_ACT, &token_rect);
	}
}

enum floor_type_t
{
	FLOOR_GRASSLAND,
};
typedef enum floor_type_t floor_type_t;

enum option_t
{
	OPTION_WALK,
	OPTION_TOWER_YELLOW,
	OPTION_TOWER_BLUE,
	OPTION_MACHINE_MULTIACT,

	OPTION_NUMBER
};
typedef enum option_t option_t;

bool option_is_tower(option_t option)
{
	switch (option)
	{
		case OPTION_TOWER_YELLOW:
		case OPTION_TOWER_BLUE:
		case OPTION_MACHINE_MULTIACT:
			return true;
		default:
			return false;
	}
}

struct tile_t
{
	floor_type_t floor;
	int sprite_variant;
	obj_t obj;
	bool is_hovered;
	bool is_selected;
	bool options[OPTION_NUMBER];
};
typedef struct tile_t tile_t;

void tile_init(tile_t* tile)
{
	*tile = (tile_t){0};

	tile->floor = FLOOR_GRASSLAND;

	tile->sprite_variant = 0;
	if (rand() % 10 == 0)
	{
		tile->sprite_variant = 1 + rand() % 3;
	}

	tile->obj = (obj_t){.type = OBJ_NONE};
	if (rand() % 10 == 0)
	{
		tile->obj.type = OBJ_TREE;
	}
	else if (rand() % 40 == 0)
	{
		tile->obj.type = OBJ_ROCK;
		tile->obj.sprite_variant = rand() % 3;
	}
}

bool tile_has_options(tile_t const* tile)
{
	for (int i = 0; i < OPTION_NUMBER; i++)
	{
		if (tile->options[i])
		{
			return true;
		}
	}
	return false;
}

bool tile_has_tower_options(tile_t const* tile)
{
	for (int i = 0; i < OPTION_NUMBER; i++)
	{
		if (option_is_tower(i) && tile->options[i])
		{
			return true;
		}
	}
	return false;
}

int tile_count_options(tile_t const* tile)
{
	int count = 0;
	for (int i = 0; i < OPTION_NUMBER; i++)
	{
		if (tile->options[i])
		{
			count++;
		}
	}
	return count;
}

void tile_clear_options(tile_t* tile)
{
	for (int i = 0; i < OPTION_NUMBER; i++)
	{
		tile->options[i] = false;
	}
}

/* The cool modulo operator, the one that gives 7 for -3 mod 10. */
int cool_mod(int a, int b)
{
	assert(b > 0);
	if (a >= 0)
	{
		return a % b;
	}
	else
	{
		return (b - ((-a) % b)) % b;
	}
}

int g_selected_option_index;

option_t tile_selected_option(tile_t const* tile)
{
	int option_index = 0;
	int const selected_option_index =
		cool_mod(g_selected_option_index, tile_count_options(tile));
	for (int i = 0; i < OPTION_NUMBER; i++)
	{
		if (tile->options[i])
		{
			if (option_index == selected_option_index)
			{
				return i;
			}
			option_index++;
		}
	}
	assert(false);
	exit(EXIT_FAILURE);
}

bool obj_type_is_tower(obj_type_t type);

/* The original color was (109, 216, 37).
 * There is also (30, 137, 77) that feels good (and it could be nice to have the color be
 * chosen at random) but some colors of map ui elements are really hard to look at in front
 * of this color. */
rgb_t g_grassland_color = {109, 216, 37};

void draw_tile(tile_t const* tile, sc_t sc, int side)
{
	assert(tile->floor == FLOOR_GRASSLAND);

	/* Draw grassland color and sprite decoration. */
	SDL_Rect rect = {.x = sc.x, .y = sc.y, .w = side, .h = side};
	SDL_SetRenderDrawColor(g_renderer,
		g_grassland_color.r, g_grassland_color.g, g_grassland_color.b, 255);
	SDL_RenderFillRect(g_renderer, &rect);
	sprite_t sprite = SPRITE_GRASSLAND_0 + tile->sprite_variant;
	draw_sprite(sprite, &rect);

	/* Draw additional UI square around the tile. */
	if (tile->is_selected)
	{
		SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 100);
		SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderFillRect(g_renderer, &rect);
		SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
	}
	else if (tile->is_hovered && !tile_has_options(tile))
	{
		SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 60);
		SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderFillRect(g_renderer, &rect);
		SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
	}
	if (tile_has_options(tile))
	{
		int const margin = tile->is_hovered ? 3 : 5;
		rect.x += margin;
		rect.y += margin;
		rect.w -= margin * 2;
		rect.h -= margin * 2;
		if (tile->options[OPTION_WALK] && tile_has_tower_options(tile))
		{
			SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
		}
		else if (tile->options[OPTION_WALK])
		{
			SDL_SetRenderDrawColor(g_renderer, 150, 100, 255, 255);
		}
		else if (tile_has_tower_options(tile))
		{
			SDL_SetRenderDrawColor(g_renderer, 255, 0, 0, 255);
		}
		else
		{
			/* h ? */
			assert(false);
		}
		SDL_RenderDrawRect(g_renderer, &rect);

		/* Draw the little option symbols. */
		if (tile_has_options(tile) &&
			(tile->is_hovered || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LALT]))
		{
			int option_index = 0;
			int const selected_option_index =
				cool_mod(g_selected_option_index, tile_count_options(tile));
			SDL_Rect option_rect = rect;
			option_rect.x += 2;
			option_rect.y += 2;
			option_rect.w = 16;
			option_rect.h = 16;
			for (int i = 0; i < OPTION_NUMBER; i++)
			{
				if (tile->options[i])
				{
					switch (i)
					{
						case OPTION_WALK:             sprite = SPRITE_WALK;             break;
						case OPTION_TOWER_YELLOW:     sprite = SPRITE_TOWER_YELLOW;     break;
						case OPTION_TOWER_BLUE:       sprite = SPRITE_TOWER_BLUE;       break;
						case OPTION_MACHINE_MULTIACT: sprite = SPRITE_MACHINE_MULTIACT; break;
						default: assert(false);
					}
					draw_sprite(sprite, &option_rect);

					if (tile->is_hovered && option_index == selected_option_index)
					{
						SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
						SDL_RenderDrawRect(g_renderer, &option_rect);
					}

					option_rect.y += 16 + 2;
					if (option_rect.y + option_rect.h + 2 > rect.y + rect.h)
					{
						option_rect.y = rect.y + 2;
						option_rect.x += 16 + 2;
					}
					option_index++;
				}
			}
		}
	}

	/* Draw the object that is on the tile. */
	draw_obj(&tile->obj, sc, side);

	/* Draw the tower ammo count. */
	if ((obj_type_is_tower(tile->obj.type) && tile->obj.ammo > 0) &&
		(tile->is_selected || tile->is_hovered || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LALT]))
	{
		SDL_Rect text_rect = rect;
		text_rect.y -= 18;
		text_rect.w = 18;
		text_rect.h = 18;
		SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
		SDL_RenderFillRect(g_renderer, &text_rect);
		sc_t text_sc = {.x = rect.x, .y = rect.y - 22};
		char string[20];
		sprintf(string, "%d", tile->obj.ammo);
		draw_text(string, (rgb_t){0, 0, 0}, text_sc);
	}

	/* Draw the life count. */
	if (tile->obj.type == OBJ_ENEMY_LEGS &&
		(tile->is_selected || tile->is_hovered || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LALT]))
	{
		SDL_Rect text_rect = rect;
		text_rect.y -= 18;
		text_rect.w = 18;
		text_rect.h = 18;
		SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
		SDL_RenderFillRect(g_renderer, &text_rect);
		sc_t text_sc = {.x = rect.x, .y = rect.y - 22};
		char string[20];
		sprintf(string, "%d", tile->obj.life);
		draw_text(string, (rgb_t){0, 0, 255}, text_sc);
	}
}

struct motion_t
{
	int t;
	int t_max;
	tc_t src, dst;
	obj_t obj;
};
typedef struct motion_t motion_t;

motion_t g_motion;

#define CHUNK_SIDE 16

struct chunk_t
{
	tc_t top_left_tc;
	tile_t* tiles;
};
typedef struct chunk_t chunk_t;

/* Tiles are stored in fixed-sized chunks, and chunks are stored in a dynaic array.
 * New chunks can be generated without having to move the existing tiles (so there
 * is no need to worry about the validity of pointers to tiles). */
int g_chunk_da_len = 0, g_chunk_da_cap = 0;
chunk_t* g_chunk_da = NULL;

bool tc_in_map(tc_t tc)
{
	for (int i = 0; i < g_chunk_da_len; i++)
	{
		chunk_t* chunk = &g_chunk_da[i];
		if (chunk->top_left_tc.x <= tc.x && tc.x < chunk->top_left_tc.x + CHUNK_SIDE &&
			chunk->top_left_tc.y <= tc.y && tc.y < chunk->top_left_tc.y + CHUNK_SIDE)
		{
			return true;
		}
	}
	return false;
}

bool g_some_chunks_are_new = false;

tile_t* map_tile(tc_t tc)
{
	/* Search in the already generated chunks first. */
	for (int i = 0; i < g_chunk_da_len; i++)
	{
		chunk_t* chunk = &g_chunk_da[i];
		if (chunk->top_left_tc.x <= tc.x && tc.x < chunk->top_left_tc.x + CHUNK_SIDE &&
			chunk->top_left_tc.y <= tc.y && tc.y < chunk->top_left_tc.y + CHUNK_SIDE)
		{
			int inchunk_x = tc.x - chunk->top_left_tc.x;
			int inchunk_y = tc.y - chunk->top_left_tc.y;
			return &chunk->tiles[inchunk_y * CHUNK_SIDE + inchunk_x];
		}
	}
	/* It seems that the requested tile was not generated yet,
	 * so we generate the chunk that contains it now. */
	g_some_chunks_are_new = true;
	DA_LENGTHEN(g_chunk_da_len += 1, g_chunk_da_cap, g_chunk_da, chunk_t);
	chunk_t* chunk = &g_chunk_da[g_chunk_da_len-1];
	chunk->top_left_tc.x = tc.x - cool_mod(tc.x, CHUNK_SIDE);
	chunk->top_left_tc.y = tc.y - cool_mod(tc.y, CHUNK_SIDE);
	#if 0
	printf("Generate chunk (%d~%d, %d~%d) for tile (%d, %d)\n",
		chunk->top_left_tc.x, chunk->top_left_tc.x + CHUNK_SIDE - 1,
		chunk->top_left_tc.y, chunk->top_left_tc.y + CHUNK_SIDE - 1,
		tc.x, tc.y);
	#endif
	chunk->tiles = calloc(CHUNK_SIDE * CHUNK_SIDE, sizeof(tile_t));
	for (int i = 0; i < CHUNK_SIDE * CHUNK_SIDE; i++)
	{
		tile_init(&chunk->tiles[i]);
	}
	int inchunk_x = tc.x - chunk->top_left_tc.x;
	int inchunk_y = tc.y - chunk->top_left_tc.y;
	return &chunk->tiles[inchunk_y * CHUNK_SIDE + inchunk_x];
}

struct tc_iter_all_t
{
	tc_t tc;
	int chunk_index;
};
typedef struct tc_iter_all_t tc_iter_all_t;

tc_iter_all_t tc_iter_all_init(void)
{
	if (g_chunk_da_len == 0)
	{
		return (tc_iter_all_t){.chunk_index = 0};
	}
	else
	{
		return (tc_iter_all_t){
			.chunk_index = 0,
			.tc = g_chunk_da[0].top_left_tc};
	}
}

void tc_iter_all_next(tc_iter_all_t* it)
{
	assert(it->chunk_index < g_chunk_da_len);
	chunk_t* chunk = &g_chunk_da[it->chunk_index];
	it->tc.x++;
	if (it->tc.x >= chunk->top_left_tc.x + CHUNK_SIDE)
	{
		it->tc.x = chunk->top_left_tc.x;
		it->tc.y++;
		if (it->tc.y >= chunk->top_left_tc.y + CHUNK_SIDE)
		{
			it->chunk_index++;
			if (it->chunk_index < g_chunk_da_len)
			{
				it->tc = g_chunk_da[it->chunk_index].top_left_tc;
			}
		}
	}
}

bool tc_iter_all_cond(tc_iter_all_t* it)
{
	return it->chunk_index < g_chunk_da_len;
}

void draw_map(void)
{
	tc_t top_left_tc = sc_to_tc((sc_t){0, 0});
	tc_t bottom_right_tc = sc_to_tc((sc_t){g_window_w-1, g_window_h-1});
	for (tc_iter_rect_t it = tc_iter_rect_init(tctc_to_rect(top_left_tc, bottom_right_tc));
		tc_iter_rect_cond(&it); tc_iter_rect_next(&it))
	{
		draw_tile(map_tile(it.tc), tc_to_sc(it.tc), g_tile_side_pixels);
	}

	if (g_motion.t_max > 0)
	{
		motion_t* m = &g_motion;
		float r = (float)m->t / (float)m->t_max;
		float x = (float)m->src.x * (1.0f - r) + (float)m->dst.x * r;
		float y = (float)m->src.y * (1.0f - r) + (float)m->dst.y * r;
		sc_t sc = {
			x * (float)g_tile_side_pixels + (float)g_tc_sc_offset.x,
			y * (float)g_tile_side_pixels + (float)g_tc_sc_offset.y};
		draw_obj(&m->obj, sc, g_tile_side_pixels);
	}
}

void map_generate(void)
{
	for (tc_iter_rect_t it = tc_iter_rect_init(tc_radius_to_rect((tc_t){0, 0}, 8));
		tc_iter_rect_cond(&it); tc_iter_rect_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		*tile = (tile_t){0};

		if (rand() % 10 == 0)
		{
			tile->sprite_variant = 1 + rand() % 3;
		}

		tile->obj = (obj_t){0};
		if (rand() % 17 == 1)
		{
			tile->obj.type = OBJ_ENEMY_BLOB;
		}
		else if (rand() % 13 == 3)
		{
			tile->obj.type = OBJ_ROCK;
			tile->obj.sprite_variant = rand() % 3;
		}
		else if (rand() % 13 == 3)
		{
			tile->obj.type = OBJ_TREE;
		}
	}

	/* Place the crystal somewhere near the middle of the map. */
	g_crystal_tc = (tc_t){
		rand() % 5 - 2,
		rand() % 5 - 2};
	map_tile(g_crystal_tc)->obj = (obj_t){.type = OBJ_CRYSTAL};

	/* Choose the 3 different unit types at random, by random elimination. */
	obj_type_t unit_types[] = {
		OBJ_UNIT_WALKER,
		OBJ_UNIT_BASIC,
		OBJ_UNIT_SHROOM,
		OBJ_UNIT_DASH};
	int unit_type_count = sizeof unit_types / sizeof (obj_type_t);
	while (unit_type_count > 3)
	{
		int eliminated_index = rand() % unit_type_count;
		unit_types[eliminated_index] = unit_types[unit_type_count - 1];
		unit_type_count--;
	}

	/* Place the 3 units around the crystal. */
	tc_t tc;
	tc = g_crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) += 1;
	map_tile(tc)->obj = (obj_t){.type = unit_types[0]};
	tc = g_crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) -= 1;
	map_tile(tc)->obj = (obj_t){.type = unit_types[1]};
	tc = g_crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) += rand() % 2 ? 2 : -2;
	map_tile(tc)->obj = (obj_t){.type = unit_types[2]};

	/* Also place a tower or a machine near the crystal. */
	tc = g_crystal_tc; tc.x += rand() % 2 ? 1 : -1; tc.y += rand() % 2 ? 1 : -1;
	map_tile(tc)->obj = (obj_t){
		.type =
			rand() % 3 == 0 ? OBJ_TOWER_YELLOW :
			rand() % 2 == 0 ? OBJ_TOWER_BLUE :
			OBJ_MACHINE_MULTIACT,
		.ammo = 2 + rand() % 2};

	/* Making sure that enemies close to the crystal are eggs so that the player
	 * has some time to take action against them. */
	for (tc_iter_rect_t it = tc_iter_rect_init(tc_radius_to_rect(g_crystal_tc, 3));
		tc_iter_rect_cond(&it); tc_iter_rect_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (tile->obj.type == OBJ_ENEMY_BLOB)
		{
			tile->obj.type = OBJ_ENEMY_EGG;
		}
	}
}

void map_clear_options(void)
{
	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_clear_options(map_tile(it.tc));
	}
}

void map_clear_hovered(void)
{
	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		map_tile(it.tc)->is_hovered = false;
	}
}

void map_clear_selected(void)
{
	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		map_tile(it.tc)->is_selected = false;
	}
}

void map_clear_can_still_act(void)
{
	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		map_tile(it.tc)->obj.can_still_act = false;
	}
}

tile_t* map_hovered_tile(void)
{
	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (tile->is_hovered)
		{
			return tile;
		}
	}
	return NULL;
}

tile_t* map_selected_tile(void)
{
	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (tile->is_selected)
		{
			return tile;
		}
	}
	return NULL;
}

/* Returns the coordinates of the given tile (that must be from the map). */
tc_t map_tile_tc(tile_t* tile)
{
	assert(tile != NULL);
	for (int i = 0; i < g_chunk_da_len; i++)
	{
		chunk_t* chunk = &g_chunk_da[i];
		if (chunk->tiles <= tile && tile < chunk->tiles + CHUNK_SIDE * CHUNK_SIDE)
		{
			int offset = tile - chunk->tiles;
			int inchunk_x = offset % CHUNK_SIDE;
			int inchunk_y = offset / CHUNK_SIDE;
			return (tc_t){
				.x = inchunk_x + chunk->top_left_tc.x,
				.y = inchunk_y + chunk->top_left_tc.y};
		}
	}
	assert(false);
	exit(EXIT_FAILURE);
}

sc_t g_mouse_sc;

bool g_shop_is_open;
void update_shop_entry_hovered(void);

void handle_mouse_motion(sc_t sc)
{
	g_mouse_sc = sc;
	if (g_shop_is_open)
	{
		update_shop_entry_hovered();
	}
	else
	{
		/* Update the hovered tile. */
		map_clear_hovered();
		tile_t* tile = map_tile(sc_to_tc(sc));
		if (tile != NULL)
		{
			tile->is_hovered = true;
		}
	}
}

/* A whole game turn is a succession of phases during which
 * a specific subset of the gameplay actually happens. */
enum phase_t
{
	PHASE_PLAYER,
	PHASE_ENEMY,
	PHASE_TOWERS,
};
typedef enum phase_t phase_t;

phase_t g_phase;

/* Can the player still place a tower in the current turn? */
bool g_tower_available;
/* The turn number, counting from one. */
int g_turn;
/* A time counter that allows to put a delay inbetween successive actions
 * that can happen automatically a bit too fast. */
int g_phase_time;
/* Has the player lost the game? */
bool g_game_over;

struct path_internal_t
{
	bool exists;
	tc_t next_tc;
	int length;
};
typedef struct path_internal_t path_internal_t;

path_internal_t path_internal(tc_t src, tc_t dst, int max_length)
{
	if (tc_eq(src, dst))
	{
		return (path_internal_t){
			.exists = true,
			.length = 0};
	}
	else if (max_length <= 0)
	{
		return (path_internal_t){.exists = false};
	}
	else
	{
		tc_t src_neighbors[4] = {
			{src.x-1, src.y},
			{src.x, src.y-1},
			{src.x+1, src.y},
			{src.x, src.y+1}};
		for (int i = 0; i < 4; i++)
		{
			if (map_tile(src_neighbors[i])->obj.type == OBJ_NONE)
			{
				path_internal_t rec = path_internal(src_neighbors[i], dst, max_length - 1);
				if (rec.exists)
				{
					return (path_internal_t){
						.exists = true,
						.next_tc = rec.next_tc,
						.length = rec.length + 1};
				}
			}
		}
		return (path_internal_t){.exists = false};
	}
}

bool path_exists(tc_t src, tc_t dst, int max_length)
{
	return path_internal(src, dst, max_length).exists;
}

tc_t path_next_tc(tc_t src, tc_t dst, int max_length)
{
	return path_internal(src, dst, max_length).next_tc;
}

bool obj_type_is_unit(obj_type_t type)
{
	switch (type)
	{
		case OBJ_UNIT_WALKER:
		case OBJ_UNIT_BASIC:
		case OBJ_UNIT_SHROOM:
		case OBJ_UNIT_DASH:
			return true;
		default:
			return false;
	}
}

void shop_click(void);

enum inventory_entry_t
{
	INVENTORY_ENTRY_MULTIACT,

	INVENTORY_ENTRY_NUMBER
};
typedef enum inventory_entry_t inventory_entry_t;

int g_inventory[INVENTORY_ENTRY_NUMBER] = {0};

int tc_dist(tc_t a, tc_t b);

void update_selected_unit_options(tc_t unit_tc)
{
	map_clear_options();

	tile_t* unit_tile = map_tile(unit_tc);
	g_selected_option_index = 0;

	/* Handle the walking for the dash unit. */
	if (unit_tile->obj.type == OBJ_UNIT_DASH)
	{
		struct dir_t
		{
			int dx, dy;
		};
		typedef struct dir_t dir_t;
		dir_t dirs[4] = {
			{0, -1},
			{1, 0},
			{0, 1},
			{-1, 0}};
		for (int i = 0; i < 4; i++)
		{
			/* This dash unit can dash in a direction if there is an obstacle behind. */
			tc_t behind_tc = unit_tc;
			behind_tc.x -= dirs[i].dx;
			behind_tc.y -= dirs[i].dy;
			if (map_tile(behind_tc)->obj.type != OBJ_NONE)
			{
				tc_t tc = unit_tc;
				while (true)
				{
					tc.x += dirs[i].dx;
					tc.y += dirs[i].dy;
					if (!tc_in_map(tc))
					{
						break;
					}
					tile_t* tile = map_tile(tc);
					if (tile->obj.type != OBJ_NONE)
					{
						break;
					}
					tile->options[OPTION_WALK] = true;
				}
			}
		}
	}

	int walk_dist = 
		unit_tile->obj.type == OBJ_UNIT_WALKER ? 4 :
		unit_tile->obj.type == OBJ_UNIT_BASIC ? 3 :
		unit_tile->obj.type == OBJ_UNIT_SHROOM ? 2 :
		unit_tile->obj.type == OBJ_UNIT_DASH ? 0 : /* Handled above. */
		(assert(false), 0);
	int tower_dist =
		unit_tile->obj.type == OBJ_UNIT_WALKER ? 1 :
		unit_tile->obj.type == OBJ_UNIT_BASIC ? 2 :
		unit_tile->obj.type == OBJ_UNIT_SHROOM ? 3 :
		unit_tile->obj.type == OBJ_UNIT_DASH ? 1 :
		(assert(false), 0);
	int const radius = max(walk_dist, tower_dist);
	for (tc_iter_rect_t it = tc_iter_rect_init(tc_radius_to_rect(unit_tc, radius));
		tc_iter_rect_cond(&it); tc_iter_rect_next(&it))
	{
		if (tc_eq(unit_tc, it.tc))
		{
			continue;
		}
		bool tower_and_machines =
			(
				unit_tile->obj.type != OBJ_UNIT_SHROOM &&
				path_exists(unit_tc, it.tc, tower_dist)) ||
			(
				unit_tile->obj.type == OBJ_UNIT_SHROOM &&
				tc_dist(unit_tc, it.tc) == 3 &&
				map_tile(it.tc)->obj.type == OBJ_NONE);
		tile_t* tile = map_tile(it.tc);
		tile->options[OPTION_WALK] |=
			path_exists(unit_tc, it.tc, walk_dist);
		tile->options[OPTION_TOWER_YELLOW] |=
			g_tower_available &&
			tower_and_machines;
		tile->options[OPTION_TOWER_BLUE] |=
			g_tower_available &&
			tower_and_machines;
		tile->options[OPTION_MACHINE_MULTIACT] |=
			g_inventory[INVENTORY_ENTRY_MULTIACT] >= 1 &&
			tower_and_machines;
	}
}

bool tile_is_adjacent_to(tc_t tc, obj_type_t to_what);
int tc_dist(tc_t a, tc_t b);

void perform_option(tc_t unit_tc, tc_t dst_tc, option_t option)
{
	tile_t* unit_tile = map_tile(unit_tc);
	switch (option)
	{
		case OPTION_WALK:
			g_motion.t = 0;
			g_motion.t_max = 7;
			g_motion.src = unit_tc;
			g_motion.dst = dst_tc;
			g_motion.obj = unit_tile->obj;
			g_motion.obj.can_still_act = false;
			unit_tile->obj = (obj_t){0};
			map_clear_options();
		break;
		case OPTION_TOWER_YELLOW:
			g_motion.t = 0;
			g_motion.t_max = 7;
			g_motion.src = unit_tc;
			g_motion.dst = dst_tc;
			g_motion.obj = (obj_t){.type = OBJ_TOWER_YELLOW, .ammo = 4};
			g_tower_available = false;
			if (!tile_is_adjacent_to(unit_tc, OBJ_MACHINE_MULTIACT))
			{
				unit_tile->obj.can_still_act = false;
			}
			map_clear_options();
		break;
		case OPTION_TOWER_BLUE:
			g_motion.t = 0;
			g_motion.t_max = 7;
			g_motion.src = unit_tc;
			g_motion.dst = dst_tc;
			g_motion.obj = (obj_t){.type = OBJ_TOWER_BLUE, .ammo = 5};
			g_tower_available = false;
			if (!tile_is_adjacent_to(unit_tc, OBJ_MACHINE_MULTIACT))
			{
				unit_tile->obj.can_still_act = false;
			}
			map_clear_options();
		break;
		case OPTION_MACHINE_MULTIACT:
			g_motion.t = 0;
			g_motion.t_max = 7;
			g_motion.src = unit_tc;
			g_motion.dst = dst_tc;
			g_motion.obj = (obj_t){.type = OBJ_MACHINE_MULTIACT};
			assert(g_inventory[INVENTORY_ENTRY_MULTIACT] >= 1);
			g_inventory[INVENTORY_ENTRY_MULTIACT]--;
			if (!(tile_is_adjacent_to(unit_tc, OBJ_MACHINE_MULTIACT) ||
				tc_dist(g_motion.src, g_motion.dst) == 1))
			{
				unit_tile->obj.can_still_act = false;
			}
			map_clear_options();
		break;
		default:
			assert(false);
		break;
	}
}

void handle_mouse_click(bool is_left_click)
{
	if (g_shop_is_open)
	{
		shop_click();
		return;
	}

	tile_t* old_selected = map_selected_tile();

	/* Update the selected tile. */
	if (is_left_click)
	{
		map_clear_selected();
	}
	tile_t* new_selected = map_hovered_tile();
	if (new_selected == NULL)
	{
		return;
	}
	if (is_left_click)
	{
		new_selected->is_selected = true;
	}

	/* If an animation is playing, then we forbid interaction. */
	if (g_motion.t_max > 0)
	{
		return;
	}

	if (is_left_click && tile_has_options(new_selected))
	{
		option_t option = tile_selected_option(new_selected);
		perform_option(map_tile_tc(old_selected), map_tile_tc(new_selected), option);
	}

	if (is_left_click)
	{
		map_clear_options();
	}
	if (obj_type_is_unit(new_selected->obj.type) &&
		new_selected->obj.can_still_act &&
		is_left_click)
	{
		update_selected_unit_options(map_tile_tc(new_selected));
	}
}

int g_shop_scroll;

void scroll_shop(int scroll)
{
	g_shop_scroll += scroll * 30;
}

/* Change the selected option on available tiles that present options. */
void scroll_option(int scroll)
{
	g_selected_option_index += -scroll;
}

void scroll_map_zoom(int scroll)
{
	/* Save the precise position in the map (tc coordinate space)
	 * where the mouse is pointing (before zooming) so that we can
	 * move the map after zooming to put that position under the
	 * mouse again (so that the zoom is on the cursor and not on
	 * the top left corner of the tile at (0, 0)). */
	sc_t mouse;
	SDL_GetMouseState(&mouse.x, &mouse.y);
	float old_float_tc_x =
		(float)(mouse.x - g_tc_sc_offset.x) / (float)g_tile_side_pixels;
	float old_float_tc_y =
		(float)(mouse.y - g_tc_sc_offset.y) / (float)g_tile_side_pixels;
	
	/* Zoom. */
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL])
	{
		/* Holding the left control key makes the zoom be pixel-per-pixel. */
		g_tile_side_pixels += scroll;
		if (g_tile_side_pixels <= 0)
		{
			g_tile_side_pixels = 1;
		}
	}
	else
	{
		/* Not holding the left control key makes the zoom be so that each pixel
		 * from a sprite is exactly mapped on an integer number of pixels on the
		 * screen. This is a sane default for now. */
		if (g_tile_side_pixels % 16 != 0)
		{
			if (scroll < 0)
			{
				g_tile_side_pixels -= g_tile_side_pixels % 16;
				scroll++;
			}
			else if (scroll > 0)
			{
				g_tile_side_pixels -= g_tile_side_pixels % 16;
				g_tile_side_pixels += 16;
				scroll--;
			}
		}
		g_tile_side_pixels += scroll * 16;
		int const max_side = min(g_window_w, g_window_h) / 2;
		if (g_tile_side_pixels < 16)
		{
			g_tile_side_pixels = 16;
		}
		else if (g_tile_side_pixels > max_side)
		{
			g_tile_side_pixels = max_side - max_side % 16;
		}
	}

	/* Moves the map to make sure that the precise point on the map pointed to
	 * by the mouse is still under the mouse after. */
	float new_float_tc_x =
		(float)(mouse.x - g_tc_sc_offset.x) / (float)g_tile_side_pixels;
	float new_float_tc_y =
		(float)(mouse.y - g_tc_sc_offset.y) / (float)g_tile_side_pixels;
	g_tc_sc_offset_frac_x +=
		(new_float_tc_x - old_float_tc_x) * (float)g_tile_side_pixels;
	g_tc_sc_offset.x += floorf(g_tc_sc_offset_frac_x);
	g_tc_sc_offset_frac_x -= floorf(g_tc_sc_offset_frac_x);
	g_tc_sc_offset_frac_y +=
		(new_float_tc_y - old_float_tc_y) * (float)g_tile_side_pixels;
	g_tc_sc_offset.y += floorf(g_tc_sc_offset_frac_y);
	g_tc_sc_offset_frac_y -= floorf(g_tc_sc_offset_frac_y);
}

void handle_mouse_wheel(int scroll)
{
	if (g_shop_is_open)
	{
		scroll_shop(scroll);
	}
	else if (tile_has_options(map_hovered_tile()))
	{
		scroll_option(scroll);
	}
	else
	{
		scroll_map_zoom(scroll);
	}
}

void handle_window_resize(int new_w, int new_h)
{
	/* Save the precise position in the map (tc coordinate space) where the
	 * center of the window (before resizing the window) is so that we can
	 * move the map after resizing the window to put that position at the
	 * center again (so that the resizing is more pleasing to the eye). */
	sc_t old_center = {g_window_w / 2, g_window_h / 2};
	float old_float_tc_x =
		(float)(old_center.x - g_tc_sc_offset.x) / (float)g_tile_side_pixels;
	float old_float_tc_y =
		(float)(old_center.y - g_tc_sc_offset.y) / (float)g_tile_side_pixels;
	
	g_window_w = new_w;
	g_window_h = new_h;

	/* Moves the map to make sure that the precise point on the map at
	 * the old center is still at the center after the resizing. */
	sc_t new_center = {g_window_w / 2, g_window_h / 2};
	float new_float_tc_x =
		(float)(new_center.x - g_tc_sc_offset.x) / (float)g_tile_side_pixels;
	float new_float_tc_y =
		(float)(new_center.y - g_tc_sc_offset.y) / (float)g_tile_side_pixels;
	g_tc_sc_offset_frac_x +=
		(new_float_tc_x - old_float_tc_x) * (float)g_tile_side_pixels;
	g_tc_sc_offset.x += floorf(g_tc_sc_offset_frac_x);
	g_tc_sc_offset_frac_x -= floorf(g_tc_sc_offset_frac_x);
	g_tc_sc_offset_frac_y +=
		(new_float_tc_y - old_float_tc_y) * (float)g_tile_side_pixels;
	g_tc_sc_offset.y += floorf(g_tc_sc_offset_frac_y);
	g_tc_sc_offset_frac_y -= floorf(g_tc_sc_offset_frac_y);
}

int tc_dist(tc_t a, tc_t b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}

bool obj_type_is_enemy(obj_type_t type)
{
	switch (type)
	{
		case OBJ_ENEMY_BLOB:
		case OBJ_ENEMY_LEGS:
		case OBJ_ENEMY_EGG:
			return true;
		default:
			return false;
	}
}

int g_enemy_already_spawn_count;
int g_enemy_big_already_spawn_count;

void spawn_one_enemy(obj_type_t type)
{
	assert(obj_type_is_enemy(type));

	/* Find a spawn location. There is a source and a destination for
	 * the little animation when spawning (as if the enemy comes from
	 * even farer). */
	int const spawn_distance = 14;
	tc_t spawn_location_src = g_crystal_tc;
	tc_t spawn_location_dst = g_crystal_tc;
	while (tc_dist(spawn_location_dst, g_crystal_tc) < spawn_distance ||
		map_tile(spawn_location_dst)->obj.type != OBJ_NONE)
	{
		spawn_location_dst = spawn_location_src;
		if (rand() % 2 == 0)
		{
			spawn_location_src.x += rand() % 3 - 1;
		}
		else
		{
			spawn_location_src.y += rand() % 3 - 1;
		}
	}

	g_motion.t = 0;
	g_motion.t_max = 6;
	g_motion.src = spawn_location_src;
	g_motion.dst = spawn_location_dst;
	g_motion.obj = (obj_t){.type = type};
	if (g_motion.obj.type == OBJ_ENEMY_LEGS)
	{
		g_motion.obj.life = 3;
	}
	g_motion.obj.can_still_act = false;

	g_enemy_already_spawn_count++;
	if (g_motion.obj.type == OBJ_ENEMY_LEGS)
	{
		g_enemy_big_already_spawn_count++;
	}
}

bool obj_type_is_tower(obj_type_t type)
{
	switch (type)
	{
		case OBJ_TOWER_YELLOW:
		case OBJ_TOWER_BLUE:
			return true;
		default:
			return false;
	}
}

bool tile_is_walkable_by_enemy(tile_t* tile)
{
	return
		tile->obj.type == OBJ_NONE ||
		tile->obj.type == OBJ_CRYSTAL ||
		obj_type_is_unit(tile->obj.type) ||
		obj_type_is_tower(tile->obj.type) ||
		tile->obj.type == OBJ_MACHINE_MULTIACT ||
		tile->obj.type == OBJ_BLOB_RED ||
		tile->obj.type == OBJ_TREE;
}

tc_t compute_enemy_move(tc_t fly_tc, int recursion_budget, bool allow_random)
{
	if (tc_eq(fly_tc, g_crystal_tc))
	{
		return fly_tc;
	}

	/* List walkable neighbors. */
	tc_t neighbor_tcs[4] = {
		{fly_tc.x-1, fly_tc.y},
		{fly_tc.x, fly_tc.y-1},
		{fly_tc.x+1, fly_tc.y},
		{fly_tc.x, fly_tc.y+1}};
	tc_t walkable_neighbor_tcs[4];
	int walkable_neighbor_count = 0;
	bool reverse = rand() % 2 == 0; /* Reverse the order of the tiles ? */
	for (int i = 0; i < 4; i++)
	{
		int index = reverse ? 3 - i : i;
		if (tile_is_walkable_by_enemy(map_tile(neighbor_tcs[index])))
		{
			walkable_neighbor_tcs[walkable_neighbor_count++] = neighbor_tcs[index];
		}
	}

	if (allow_random ? rand() % 20 != 0 : true)
	{
		for (int i = 0; i < walkable_neighbor_count; i++)
		{
			int new_dist = tc_dist(walkable_neighbor_tcs[i], g_crystal_tc);
			if (new_dist < tc_dist(fly_tc, g_crystal_tc))
			{
				return walkable_neighbor_tcs[i];
			}
		}
		if (recursion_budget >= 1)
		{
			/* Try investigating where moving elsewhere can get us. */
			for (int i = 0; i < walkable_neighbor_count; i++)
			{
				tc_t next_next_tc = compute_enemy_move(compute_enemy_move(walkable_neighbor_tcs[i],
					recursion_budget-1, false), recursion_budget-1, false);
				int next_next_new_dist = tc_dist(next_next_tc, g_crystal_tc);
				if (next_next_new_dist < tc_dist(fly_tc, g_crystal_tc))
				{
					return walkable_neighbor_tcs[i];
				}
			}
		}
	}
	if (walkable_neighbor_count > 0)
	{
		return walkable_neighbor_tcs[rand() % walkable_neighbor_count];
	}
	else
	{
		return fly_tc;
	}
}

int g_enemy_count;
int g_enemy_that_played_count;

bool game_play_enemy(void)
{
	for (tc_iter_all_spiral_t it = tc_iter_all_spiral_init(g_crystal_tc);
		tc_iter_all_spiral_cond(&it); tc_iter_all_spiral_next(&it))
	{
		tc_t src_tc = it.tc;
		tile_t* src_tile = map_tile(src_tc);
		if (src_tile->obj.can_still_act)
		{
			if (src_tile->obj.type == OBJ_ENEMY_EGG)
			{
				/* All an egg can do is hatch by chance. */
				if (tile_is_adjacent_to(src_tc, OBJ_MACHINE_MULTIACT))
				{
					src_tile->obj.type = OBJ_ENEMY_BLOB;
					return false;
				}
				src_tile->obj.can_still_act = false;
				g_enemy_that_played_count++;
				if (rand() % 4 == 0)
				{
					src_tile->obj.type = OBJ_ENEMY_BLOB;
				}
				return false;
			}
			else if (src_tile->obj.type == OBJ_ENEMY_BLOB ||
				src_tile->obj.type == OBJ_ENEMY_LEGS)
			{
				/* A fly can move or lay an egg to an adjacent tile,
				 * which is to be decided. */
				bool lay_egg = rand() % 11 == 0;
				
				tc_t dst_tc = compute_enemy_move(src_tc, 2, true);
				if (src_tile->obj.type == OBJ_ENEMY_LEGS &&
					rand() % 17 != 0)
				{
					dst_tc = compute_enemy_move(dst_tc, 2, true);
				}

				if (!tc_in_map(dst_tc) || tc_eq(src_tc, dst_tc))
				{
					src_tile->obj.can_still_act = false;
					g_enemy_that_played_count++;
					return false;
				}
				tile_t* dst_tile = map_tile(dst_tc);

				if (!tile_is_walkable_by_enemy(dst_tile))
				{
					src_tile->obj.can_still_act = false;
					g_enemy_that_played_count++;
					return false;
				}


				if (lay_egg)
				{
					g_motion.t = 0;
					g_motion.t_max = 6;
					g_motion.src = src_tc;
					g_motion.dst = dst_tc;
					g_motion.obj = (obj_t){.type = OBJ_ENEMY_EGG};
					if (!tile_is_adjacent_to(src_tc, OBJ_MACHINE_MULTIACT))
					{
						src_tile->obj.can_still_act = false;
						g_enemy_that_played_count++;
					}
				}
				else
				{
					g_motion.t = 0;
					g_motion.t_max = 6;
					g_motion.src = src_tc;
					g_motion.dst = dst_tc;
					g_motion.obj = src_tile->obj;
					g_motion.obj.can_still_act = false;
					src_tile->obj = (obj_t){0};
					g_enemy_that_played_count++;
				}
				return false;
			}
			else
			{
				assert(false);
			}
		}
	}

	int const enemy_spawn_number =
		g_turn <  5 ? 0 + (g_turn % 2) :
		g_turn < 10 ? 1 :
		g_turn < 15 ? 1 + (g_turn % 2) :
		g_turn < 20 ? 2 :
		g_turn < 25 ? 2 + (g_turn % 2) :
		g_turn < 30 ? 3 :
		g_turn < 35 ? 3 + (g_turn % 2) :
		4;
	while (g_enemy_already_spawn_count < enemy_spawn_number)
	{
		spawn_one_enemy(OBJ_ENEMY_BLOB);
		return false;
	}

	int const enemy_big_spawn_number = (g_turn+1) % 5 == 0 ? 1 : 0;
	if (g_enemy_big_already_spawn_count < enemy_big_spawn_number)
	{
		spawn_one_enemy(OBJ_ENEMY_LEGS);
		return false;
	}

	return true;
}

int g_tower_count;
int g_tower_that_played_count;

void tower_shoot(tc_t tower_tc, tc_t target_tc, obj_type_t shot_type)
{
	g_motion.t = 0;
	g_motion.t_max = 12;
	g_motion.src = tower_tc;
	g_motion.dst = target_tc;
	g_motion.obj = (obj_t){.type = shot_type};

	tile_t* tower_tile = map_tile(tower_tc);
	assert(tower_tile->obj.ammo > 0);
	tower_tile->obj.ammo--;
}

void tower_is_done(tc_t tower_tc)
{
	tile_t* tower_tile = map_tile(tower_tc);
	tower_tile->obj.can_still_act = false;
	g_tower_that_played_count++;
}

bool tile_is_adjacent_to(tc_t tc, obj_type_t to_what)
{
	struct dir_t
	{
		int dx, dy;
	};
	typedef struct dir_t dir_t;
	dir_t dirs[4] = {
		{0, -1},
		{1, 0},
		{0, 1},
		{-1, 0}};
	for (int i = 0; i < 4; i++)
	{
		tc_t neighbor = tc;
		neighbor.x += dirs[i].dx;
		neighbor.y += dirs[i].dy;
		if (map_tile(neighbor)->obj.type == to_what)
		{
			return true;
		}
	}
	return false;
}

int g_tower_orientation;

bool game_play_towers(void)
{
	for (tc_iter_all_spiral_t it = tc_iter_all_spiral_init(g_crystal_tc);
		tc_iter_all_spiral_cond(&it); tc_iter_all_spiral_next(&it))
	{
		tc_t src_tc = it.tc;
		tile_t* tower_tile = map_tile(src_tc);
		if (tower_tile->obj.can_still_act)
		{
			obj_type_t tower_type = tower_tile->obj.type;
			assert(obj_type_is_tower(tower_type) && tower_tile->obj.ammo > 0);

			/* If the tower is adjacent to a multiact machine (which we determine now)
			 * then it will be able to shoot as many times as it wants. */
			bool multiact = tile_is_adjacent_to(src_tc, OBJ_MACHINE_MULTIACT);

			struct dir_t
			{
				int dx, dy;
				tc_t tc;
				bool is_valid;
			};
			typedef struct dir_t dir_t;
			dir_t dirs[4];
			for (int i = 0; i < 4; i ++)
			{
				dirs[i] =
					(dir_t[4]){
						{.dx = 0, .dy = -1},
						{.dx = 1, .dy = 0},
						{.dx = 0, .dy = 1},
						{.dx = -1, .dy = 0}
					}[(g_tower_orientation + i) % 4];
				dirs[i].tc = src_tc;
				if (tower_type == OBJ_TOWER_YELLOW)
				{
					/* A yellow tower cannot shoot at a distance of one tile. */
					dirs[i].tc.x += dirs[i].dx;
					dirs[i].tc.y += dirs[i].dy;
					dirs[i].is_valid =
						tc_in_map(dirs[i].tc) &&
						map_tile(dirs[i].tc)->obj.type == OBJ_NONE;
				}
				else
				{
					dirs[i].is_valid = tc_in_map(dirs[i].tc);
				}
			}

			bool done = false;
			while (!done)
			{
				done = true;
				for (int i = 0; i < 4; i ++)
				{
					if (!dirs[i].is_valid)
					{
						continue;
					}
					done = false;

					dirs[i].tc.x += dirs[i].dx;
					dirs[i].tc.y += dirs[i].dy;
					if (!tc_in_map(dirs[i].tc))
					{
						dirs[i].is_valid = false;
						continue;
					}
					obj_type_t type = map_tile(dirs[i].tc)->obj.type;
					if (obj_type_is_enemy(type))
					{
						obj_type_t shot_type =
							tower_type == OBJ_TOWER_YELLOW ? OBJ_SHOT_BLUE :
							tower_type == OBJ_TOWER_BLUE ? OBJ_SHOT_RED :
							(assert(false), 0);
						tower_shoot(src_tc, dirs[i].tc, shot_type);
						if (!multiact || tower_tile->obj.ammo <= 0)
						{
							tower_is_done(src_tc);
						}
						return false;
					}
					if (type != OBJ_NONE)
					{
						dirs[i].is_valid = false;
					}
				}
			}

			tower_is_done(src_tc);
			return false;
		}
	}

	return true;
}

void start_enemy_phase(void)
{
	g_phase = PHASE_ENEMY;
	g_phase_time = 0;
	g_enemy_already_spawn_count = 0;
	g_enemy_big_already_spawn_count = 0;
	map_clear_options();
	map_clear_selected();
	map_clear_can_still_act();

	g_enemy_count = 0;
	g_enemy_that_played_count = 0;
	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (obj_type_is_enemy(tile->obj.type))
		{
			tile->obj.can_still_act = true;
			g_enemy_count++;
		}
	}
}

void start_tower_phase(void)
{
	g_phase = PHASE_TOWERS;
	g_phase_time = 0;
	map_clear_options();
	map_clear_selected();
	map_clear_can_still_act();

	g_tower_count = 0;
	g_tower_that_played_count = 0;
	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (obj_type_is_tower(tile->obj.type) && tile->obj.ammo > 0)
		{
			tile->obj.can_still_act = true;
			g_tower_count++;
		}
	}
}

void start_player_phase(void)
{
	if (g_game_over)
	{
		start_enemy_phase();
		return;
	}

	g_turn++;
	if (g_turn == 1)
	{
		g_tower_orientation = 0;
	}
	else
	{
		g_tower_orientation = (g_tower_orientation + 1) % 4;
	}
	g_phase = PHASE_PLAYER;
	g_tower_available = true;
	g_phase_time = 0;
	map_clear_can_still_act();

	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (obj_type_is_unit(tile->obj.type))
		{
			tile->obj.can_still_act = true;
		}
	}
}

int g_act_token_count;

void close_shop(void);

void end_player_phase(void)
{
	if (g_shop_is_open)
	{
		close_shop();
	}

	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (obj_type_is_unit(tile->obj.type) && tile->obj.can_still_act)
		{
			g_act_token_count++;
		}
	}

	start_enemy_phase();
}

void game_perform(void)
{
	if (g_motion.t_max > 0)
	{
		/* An animaion is playing, it takes priority. */
		if (g_motion.t >= g_motion.t_max)
		{
			/* The animation is over and its result must be applied to the map. */
			tile_t* dst_tile = map_tile(g_motion.dst);
			if (dst_tile->obj.type == OBJ_CRYSTAL)
			{
				g_game_over = true;
				printf("Crystal destroyed, game over on turn %d\n", g_turn);
			}
			if (g_motion.obj.type == OBJ_SHOT_BLUE || g_motion.obj.type == OBJ_SHOT_RED)
			{
				/* Doing damage to the impacted tile, often killing its content. */
				if (dst_tile->obj.type == OBJ_ENEMY_LEGS)
				{
					dst_tile->obj.life--;
					if (dst_tile->obj.life <= 0)
					{
						dst_tile->obj = (obj_t){0};
					}
				}
				else
				{
					dst_tile->obj = (obj_t){0};
				}

				if (g_motion.obj.type == OBJ_SHOT_RED)
				{
					/* Try placing red blob on tiles on and after the impact. */
					int dx =
						g_motion.src.x < g_motion.dst.x ? 1 :
						g_motion.src.x > g_motion.dst.x ? -1 :
						0;
					int dy =
						g_motion.src.y < g_motion.dst.y ? 1 :
						g_motion.src.y > g_motion.dst.y ? -1 :
						0;
					tc_t tc = g_motion.dst;
					for (int i = 0; i < 2; i++)
					{
						tile_t* tile = map_tile(tc);
						if (tile->obj.type == OBJ_NONE)
						{
							tile->obj.type = OBJ_BLOB_RED;
						}
						tc.x += dx;
						tc.y += dy;
					}
				}
			}
			else
			{
				/* The motion was just an object moving. */
				dst_tile->obj = g_motion.obj;

				/* If it lands near a multiact machine, then it has to be marked
				 * as still able to act again. */
				if (obj_type_is_unit(dst_tile->obj.type) &&
					g_phase == PHASE_PLAYER &&
					tile_is_adjacent_to(g_motion.dst, OBJ_MACHINE_MULTIACT))
				{
					dst_tile->obj.can_still_act = true;
				}
				else if (obj_type_is_enemy(dst_tile->obj.type) &&
					g_phase == PHASE_ENEMY &&
					tile_is_adjacent_to(g_motion.dst, OBJ_MACHINE_MULTIACT))
				{
					dst_tile->obj.can_still_act = true;
					g_enemy_that_played_count--;
				}
			}
			g_motion = (motion_t){0};
		}
		g_motion.t++;
	}
	else if (g_phase == PHASE_ENEMY)
	{
		g_phase_time++;
		if (g_phase_time % 3 == 0)
		{
			bool const enemy_is_done = game_play_enemy();
			if (enemy_is_done)
			{
				start_tower_phase();
			}
		}
	}
	else if (g_phase == PHASE_TOWERS)
	{
		g_phase_time++;
		if (g_phase_time % 3 == 0)
		{
			bool const towers_are_done = game_play_towers();
			if (towers_are_done)
			{
				start_player_phase();
			}
		}
	}
}

void center_view(tc_t tc)
{
	sc_t sc = tc_to_sc(tc);
	g_tc_sc_offset.x -= sc.x + g_tile_side_pixels / 2 - g_window_w / 2;
	g_tc_sc_offset.y -= sc.y + g_tile_side_pixels / 2 - g_window_h / 2;
}

void draw_crystal_spiral(void)
{
	tc_t last_tc = g_crystal_tc;
	sc_t last_sc = tc_to_sc_center(last_tc);
	int order = 0;
	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
	for (tc_iter_all_spiral_t it = tc_iter_all_spiral_init(g_crystal_tc);
		tc_iter_all_spiral_cond(&it); tc_iter_all_spiral_next(&it))
	{
		sc_t sc = tc_to_sc_center(it.tc);
		if (tc_dist(last_tc, it.tc) <= 2 && !tc_eq(it.tc, last_tc))
		{
			SDL_RenderDrawLine(g_renderer, last_sc.x, last_sc.y, sc.x, sc.y);

			/* Draw the order number on the last tile (and not on the current one)
			 * to avoid the line from the current tile to the next to be drawn above
			 * the text being drawn here. */
			char string[20];
			sprintf(string, "%d", order);
			draw_text_ex(string, (rgb_t){100, 255, 255},
				last_sc, true, true,
				true, (rgb_t){0, 0, 0});
			order++;
		}
		last_tc = it.tc;
		last_sc = sc;
	}
}

enum shop_entry_t
{
	SHOP_ENTRY_TEST,
	SHOP_ENTRY_MULTIACT,

	SHOP_ENTRY_NUMBER
};
typedef enum shop_entry_t shop_entry_t; 

struct shop_entry_def_t
{
	int cost;
	char* name;
	char* description;
};
typedef struct shop_entry_def_t shop_entry_def_t;

SDL_Rect shop_entry_rect(shop_entry_t entry)
{
	int y = g_shop_scroll + 60 + 40 * entry;
	return (SDL_Rect){.x = 60, .y = y, .w = g_window_w - 120, .h = 35};
}

/* Which shop entry is hovered.
 * Set to -1 when no shop entry is hovered. */
int g_shop_entry_hovered;

void update_shop_entry_hovered(void)
{
	for (int i = 0; i < SHOP_ENTRY_NUMBER; i++)
	{
		SDL_Rect rect = shop_entry_rect(i);
		if (rect.x <= g_mouse_sc.x && g_mouse_sc.x < rect.x + rect.w &&
			rect.y <= g_mouse_sc.y && g_mouse_sc.y < rect.y + rect.h)
		{
			g_shop_entry_hovered = i;
			return;
		}
	}
	g_shop_entry_hovered = -1;
}

void draw_shop_entry(shop_entry_def_t* def, shop_entry_t entry)
{
	SDL_Rect rect = shop_entry_rect(entry);
	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(g_renderer, &rect);

	sc_t sc = {.x = rect.x + 5, .y = rect.y + 5};
	rgb_t color = g_shop_entry_hovered == (int)entry ?
		(rgb_t){255, 255, 0} :
		(rgb_t){255, 255, 255};
	draw_text(def->name, color, sc);

	sc.x = rect.x + rect.w - 80;
	char string[30];
	sprintf(string, "AT %d", def->cost);
	color = g_shop_entry_hovered == (int)entry ?
		def->cost > g_act_token_count ? (rgb_t){255, 0, 0} : (rgb_t){0, 255, 0} :
		(rgb_t){255, 255, 255};
	draw_text(string, color, sc);
}

shop_entry_def_t g_shop_table[] = {
	[SHOP_ENTRY_TEST] = {.cost = 3, .name = "PRINT \"TEST\" (NOT WORTH IT)"},
	[SHOP_ENTRY_MULTIACT] = {.cost = 20, .name = "MULTIACT MACHINE"}};

void draw_shop(void)
{
	for (int i = 0; i < SHOP_ENTRY_NUMBER; i++)
	{
		draw_shop_entry(&g_shop_table[i], i);
	}
}

void shop_click(void)
{
	assert(g_shop_is_open);

	if (g_shop_entry_hovered == -1)
	{
		close_shop();
		return;
	}

	if (g_shop_table[g_shop_entry_hovered].cost > g_act_token_count)
	{
		return;
	}

	switch (g_shop_entry_hovered)
	{
		case SHOP_ENTRY_TEST:
			printf("TEST\n");
		break;
		case SHOP_ENTRY_MULTIACT:
			g_inventory[INVENTORY_ENTRY_MULTIACT]++;
		break;
		default:
			assert(false);
		break;
	}

	g_act_token_count -= g_shop_table[g_shop_entry_hovered].cost;
}

void open_shop(void)
{
	if (g_game_over)
	{
		return;
	}

	map_clear_hovered();
	g_shop_is_open = true;
	update_shop_entry_hovered();
}

void close_shop(void)
{
	g_shop_is_open = false;

	/* Make the actually hovered tile marked as hovered again without requiring
	 * the mouse to be moved. */
	handle_mouse_motion(g_mouse_sc);
}

int main(int argc, char const* const* argv)
{
	printf("Why Crystals ? version 0.0.4 indev\n");

	g_debug = false;

	bool test_big = false;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--test-big") == 0)
		{
			test_big = true;
		}
	}

	srand(time(NULL));

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		assert(false);
	}

	g_window = SDL_CreateWindow("Why Crystals ?",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_window_w, g_window_h,
		SDL_WINDOW_RESIZABLE);
	assert(g_window != NULL);

	g_renderer = SDL_CreateRenderer(g_window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	assert(g_renderer != NULL);

	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG)
	{
		assert(false);
	}

	if (TTF_Init() < 0)
	{
		assert(false);
	}

	SDL_RWops* rwops_font = SDL_RWFromConstMem(
		g_asset_font,
		g_asset_font_size);
	assert(rwops_font != NULL);
	g_font = TTF_OpenFontRW(rwops_font, 0, 20);
	assert(g_font != NULL);

	init_sprite_sheet();

	if (rand() % 10 == 0)
	{
		g_grassland_color.r = 30;
		g_grassland_color.g = 137;
		g_grassland_color.b = 77;
	}
	
	g_motion = (motion_t){0};
	map_generate();
	center_view(g_crystal_tc);

	if (test_big)
	{
		for (tc_iter_all_t it = tc_iter_all_init();
			tc_iter_all_cond(&it); tc_iter_all_next(&it))
		{
			tile_t* tile = map_tile(it.tc);
			if (tile->obj.type == OBJ_ENEMY_BLOB)
			{
				tile->obj.type = OBJ_ENEMY_LEGS;
				tile->obj.life = 3;
			}
		}
	}

	g_shop_is_open = false;
	g_shop_scroll = 0;

	g_act_token_count = 0;
	g_turn = 0;
	g_game_over = false;
	start_player_phase();

	printf("Hold the LEFT CONTROL key to display the controls\n");

	/* Gaming. */

	g_time = 0;
	bool running = true;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					running = false;
				break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							running = false;
						break;
						case SDLK_RETURN:
							if (g_phase == PHASE_PLAYER)
							{
								end_player_phase();
							}
						break;
						case SDLK_c:
							center_view(g_crystal_tc);
						break;
						case SDLK_SPACE:
							if (g_shop_is_open)
							{
								close_shop();
							}
							else
							{
								open_shop();
							}
						break;
						case SDLK_d:
							g_debug = true;
						break;
						case SDLK_a:
						case SDLK_q:
							if (g_debug)
							{
								tile_t* selected = map_selected_tile();
								if (selected != NULL)
								{
									int incr = event.key.keysym.sym == SDLK_a ? 1 : -1;
									selected->obj.type =
										cool_mod(selected->obj.type + incr, OBJ_NUMBER);
								}
							}
						break;
						case SDLK_p:
							if (g_debug)
							{
								for (tc_iter_all_t it = tc_iter_all_init();
									tc_iter_all_cond(&it); tc_iter_all_next(&it))
								{
									tile_t* tile = map_tile(it.tc);
									tile->obj = (obj_t){0};
								}
							}
						break;
						case SDLK_t:
							if (g_debug)
							{
								g_act_token_count += 1000;
							}
						break;
					}
				break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_LEAVE:
							map_clear_hovered();
						break;
						case SDL_WINDOWEVENT_RESIZED:
							handle_window_resize(event.window.data1, event.window.data2);
						break;
					}
				break;
				case SDL_MOUSEWHEEL:
					handle_mouse_wheel(event.wheel.y);
				break;
				case SDL_MOUSEMOTION:
					if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT))
					{
						g_tc_sc_offset.x += event.motion.xrel;
						g_tc_sc_offset.y += event.motion.yrel;
					}
					handle_mouse_motion((sc_t){event.motion.x, event.motion.y});
				break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_MIDDLE && g_phase == PHASE_PLAYER)
					{
						end_player_phase();
					}
					if (g_phase == PHASE_PLAYER)
					{
						handle_mouse_click(event.button.button == SDL_BUTTON_LEFT);
					}
				break;
			}
		}

		game_perform();

		if (g_some_chunks_are_new)
		{
			g_some_chunks_are_new = false;

			/* When new chunks are generated, it can be useful to recompute the options
			 * of the selected unit (if any). For example, there is no limit to the range
			 * of the dash unit, so it may be able to move in the newly generated chunks
			 * (but they were not generated yet when making the walkable tiles). */
			tile_t* selected_tile = map_selected_tile();
			if (selected_tile != NULL &&
				obj_type_is_unit(selected_tile->obj.type) &&
				selected_tile->obj.can_still_act)
			{
				update_selected_unit_options(map_tile_tc(selected_tile));
			}
		}

		SDL_SetRenderDrawColor(g_renderer, 30, 40, 80, 255);
		SDL_RenderClear(g_renderer);
		
		draw_map();

		if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LSHIFT])
		{
			draw_crystal_spiral();
		}

		char string[200];
		int y = 0;
		#define DRAW_TEXT(...) \
			snprintf(string, sizeof string, __VA_ARGS__); \
			draw_text(string, (rgb_t){0, 0, 0}, (sc_t){0, y}); \
			y += 20
		if (g_debug)
		{
			DRAW_TEXT("DEBUG");
		}
		DRAW_TEXT("TURN %d", g_turn);
		if (g_game_over)
		{
			DRAW_TEXT("GAME OVER");
		}
		else if (g_phase == PHASE_PLAYER)
		{
			DRAW_TEXT("AT %d", g_act_token_count);
			if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LALT])
			{
				sprite_t orientation_sprites[] = {
					SPRITE_ARROW_UP, SPRITE_ARROW_RIGHT, SPRITE_ARROW_DOWN, SPRITE_ARROW_LEFT};
				DRAW_TEXT("TOWER SCAN:");
				SDL_Rect rect = {.x = 5, .y = y, .w = 32, .h = 32};
				for (int i = 0; i < 4; i++)
				{
					sprite_t sprite = orientation_sprites[(g_tower_orientation + i) % 4];
					draw_sprite(sprite, &rect);
					rect.x += 32 + 5;
				}
				y += rect.h + 5;
			}
		}
		else if (g_phase == PHASE_ENEMY)
		{
			DRAW_TEXT("ENEMY PHASE %d / %d", g_enemy_that_played_count, g_enemy_count);
		}
		else if (g_phase == PHASE_TOWERS)
		{
			DRAW_TEXT("TOWERS PHASE %d / %d", g_tower_that_played_count, g_tower_count);
		}
		if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL])
		{
			DRAW_TEXT("HOLD RIGHT-CLICK: MOVE MAP");
			DRAW_TEXT("LEFT-CLICK: SELECT TILE");
			DRAW_TEXT("LEFT-CLICK ON AVAILABLE TILE: PERFORM SELECTED ACTION");
			DRAW_TEXT("WHEEL ON AVAILABLE TILE: CHANGE SELECTED ACTION");
			DRAW_TEXT("HOLD LEFT-ALT: DISPLAY INFO");
			DRAW_TEXT("HOLD LEFT-CONTROL: DISPLAY CONTROLS");
			DRAW_TEXT("C KEY: CENTER VIEW ON CRYSTAL");
			DRAW_TEXT("WHEEL: ZOOM (PIXEL PERFECT)");
			DRAW_TEXT("WHEEL + LEFT-CONTROL: ZOOM (SLOW)");
			DRAW_TEXT("WHEEL BUTTON / ENTER: END TURN");
			DRAW_TEXT("SPACE: TOGGLE SHOP");
			DRAW_TEXT("HOLD LEFT-SHIFT: DISPLAY CRYSTAL ORDER FIELD");
			DRAW_TEXT("ESCAPE: QUIT");
		}
		#undef DRAW_TEXT

		if (g_shop_is_open)
		{
			draw_shop();
		}
		
		SDL_RenderPresent(g_renderer);
		g_time++;
	}

	if (!g_game_over)
	{
		printf("Quit on turn %d\n", g_turn);
	}
	if (g_debug)
	{
		printf("Debug was enabled\n");
	}

	TTF_Quit();
	IMG_Quit();
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}
