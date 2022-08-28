
#include "embedded.h"
#include "rendering.h"
#include "sprites.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

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
		tc_t spiral_tc = {.x = it->center.x - dist, .y = it->center.y};
		tc_t ring_end_tc = dist == 0 ? it->center :
			(tc_t){.x = it->center.x - dist + 1, .y = it->center.y + 1};
		bool ring_is_out = true;

		struct dir_t { int dx, dy; };
		typedef struct dir_t dir_t;
		dir_t dirs[] = {{1, -1}, {1, 1}, {-1, 1}, {-1, -1}};
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < dist || dist == 0; j++)
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
					spiral_tc = (tc_t){.x = it->center.x - (dist+1), .y = it->center.y};
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
#define WINDOW_SIDE (800 - 800 % 64)

/* What are the screen coordinates of top left corner of the tile at (0, 0) ? */
sc_t g_tc_sc_offset = {0};

float g_tc_sc_offset_frac_x = 0.0f, g_tc_sc_offset_frac_y = 0.0f;

sc_t tc_to_sc(tc_t tc)
{
	return (sc_t){
		tc.x * g_tile_side_pixels + g_tc_sc_offset.x,
		tc.y * g_tile_side_pixels + g_tc_sc_offset.y};
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

void draw_text(char const* text, rgb_t color, sc_t sc, bool center_x)
{
	SDL_Color sdl_color = {.r = color.r, .g = color.g, .b = color.b, .a = 255};
	SDL_Surface* surface = TTF_RenderText_Solid(g_font, text, sdl_color);
	assert(surface != NULL);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(texture != NULL);
	SDL_FreeSurface(surface);
	SDL_Rect rect = {.y = sc.y};
	SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
	rect.x = center_x ? sc.x - rect.w/2 : sc.x;
	SDL_RenderCopy(g_renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
}

enum obj_type_t
{
	OBJ_NONE = 0,
	OBJ_ROCK,
	OBJ_TREE,
	OBJ_CRYSTAL,
	OBJ_UNIT_RED,
	OBJ_UNIT_BLUE,
	OBJ_UNIT_PINK,
	OBJ_TOWER,
	OBJ_TOWER_OFF,
	OBJ_SHOT,
	OBJ_ENEMY_EGG,
	OBJ_ENEMY_FLY,
	OBJ_ENEMY_BIG,
};
typedef enum obj_type_t obj_type_t;

struct obj_t
{
	obj_type_t type;
	bool can_still_act;
	int ammo;
};
typedef struct obj_t obj_t;

void draw_obj(obj_t const* obj, sc_t sc, int side)
{
	/* Draw the object sprite. */
	sprite_t sprite;
	switch (obj->type)
	{
		case OBJ_NONE:      return;
		case OBJ_ROCK:      sprite = SPRITE_ROCK;      break;
		case OBJ_TREE:      sprite = SPRITE_TREE;      break;
		case OBJ_CRYSTAL:   sprite = SPRITE_CRYSTAL;   break;
		case OBJ_UNIT_RED:  sprite = SPRITE_UNIT_RED;  break;
		case OBJ_UNIT_BLUE: sprite = SPRITE_UNIT_BLUE; break;
		case OBJ_UNIT_PINK: sprite = SPRITE_UNIT_PINK; break;
		case OBJ_TOWER:     sprite = SPRITE_TOWER;     break;
		case OBJ_TOWER_OFF: sprite = SPRITE_TOWER_OFF; break;
		case OBJ_SHOT:      sprite = SPRITE_SHOT;      break;
		case OBJ_ENEMY_EGG: sprite = SPRITE_ENEMY_EGG; break;
		case OBJ_ENEMY_FLY: sprite = SPRITE_ENEMY_FLY; break;
		case OBJ_ENEMY_BIG: sprite = SPRITE_ENEMY_BIG; break;
		default: assert(false);
	}
	SDL_Rect rect = {.x = sc.x, .y = sc.y, .w = side, .h = side};
	draw_sprite(sprite, &rect);

	/* Draw the can-still-act indicator. */
	if (obj->can_still_act)
	{
		rect.x += 4;
		rect.y += 4;
		rect.w = 8;
		rect.h = 8;
		SDL_SetRenderDrawColor(g_renderer, 0, 100, 0, 255);
		SDL_RenderDrawRect(g_renderer, &rect);
	}
}

enum floor_type_t
{
	FLOOR_GRASSLAND,
	FLOOR_DESERT,
	FLOOR_WATER,
};
typedef enum floor_type_t floor_type_t;

enum option_t
{
	OPTION_WALK,
	OPTION_TOWER,

	OPTION_NUMBER,
};
typedef enum option_t option_t;

struct tile_t
{
	floor_type_t floor;
	obj_t obj;
	bool is_hovered;
	bool is_selected;
	bool options[OPTION_NUMBER];
};
typedef struct tile_t tile_t;

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

void tile_clear_options(tile_t* tile)
{
	for (int i = 0; i < OPTION_NUMBER; i++)
	{
		tile->options[i] = false;
	}
}

void draw_tile(tile_t const* tile, sc_t sc, int side)
{
	/* Draw the floor tile sprite. */
	sprite_t sprite;
	switch (tile->floor)
	{
		case FLOOR_GRASSLAND: sprite = SPRITE_GRASSLAND; break;
		case FLOOR_DESERT:    sprite = SPRITE_DESERT;    break;
		case FLOOR_WATER:     sprite = SPRITE_WATER;     break;
		default: assert(false);
	}
	SDL_Rect rect = {.x = sc.x, .y = sc.y, .w = side, .h = side};
	if (tile->floor == FLOOR_WATER)
	{
		rect.y += 3 * (g_tile_side_pixels / 16);
	}
	draw_sprite(sprite, &rect);

	if (tile->floor != FLOOR_WATER)
	{
		rect.y += g_tile_side_pixels;
		draw_sprite(SPRITE_SIDE_DIRT, &rect);
		rect.y -= g_tile_side_pixels;
	}

	/* Apply a color filter on the tile sprite to make them be
	 * less detailed because we see too much the details of these
	 * and it does not look good (for now). */
	/* TODO: Apply the correction on the sprite sheet asset instead
	 * of here. */
	switch (tile->floor)
	{
		case FLOOR_GRASSLAND:
			SDL_SetRenderDrawColor(g_renderer, 0, 255, 0, 180);
		break;
		case FLOOR_DESERT:
			SDL_SetRenderDrawColor(g_renderer, 255, 255, 0, 110);
		break;
		default:
			SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0);
		break;
	}
	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(g_renderer, &rect);
	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

	/* Draw additional UI square around the tile. */
	if (tile->is_selected)
	{
		SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(g_renderer, &rect);
		//draw_sprite(SPRITE_SELECT, &rect);
	}
	else if (tile->is_hovered && !tile_has_options(tile))
	{
		SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(g_renderer, &rect);
		//draw_sprite(SPRITE_HOVER, &rect);
	}
	if (tile_has_options(tile))
	{
		int const margin = tile->is_hovered ? 3 : 5;
		rect.x += margin;
		rect.y += margin;
		rect.w -= margin * 2;
		rect.h -= margin * 2;
		if (tile->options[OPTION_WALK] && tile->options[OPTION_TOWER])
		{
			SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
		}
		else if (tile->options[OPTION_WALK])
		{
			SDL_SetRenderDrawColor(g_renderer, 150, 100, 255, 255);
		}
		else if (tile->options[OPTION_TOWER])
		{
			SDL_SetRenderDrawColor(g_renderer, 255, 0, 0, 255);
		}
		else
		{
			assert(false);
		}
		SDL_RenderDrawRect(g_renderer, &rect);

		/* Draw the little option symbols. */
		if (tile->is_hovered || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LALT])
		{
			rect.w = 16;
			rect.h = 16;
			for (int i = 0; i < OPTION_NUMBER; i++)
			{
				if (tile->options[i])
				{
					switch (i)
					{
						case OPTION_WALK:  sprite = SPRITE_WALK;  break;
						case OPTION_TOWER: sprite = SPRITE_TOWER; break;
						default: assert(false);
					}
					draw_sprite(sprite, &rect);
					rect.x += 17;
				}
			}
		}
	}

	/* Draw the object that is on the tile. */
	draw_obj(&tile->obj, sc, side);

	/* Draw the tower ammo count. */
	if (tile->obj.type == OBJ_TOWER &&
		(tile->is_selected || tile->is_hovered || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LALT]))
	{
		char string[20];
		sprintf(string, "%d", tile->obj.ammo);
		draw_text(string, (rgb_t){0, 0, 0}, (sc_t){rect.x + 3, rect.y - 1}, false);
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
	DA_LENGTHEN(g_chunk_da_len += 1, g_chunk_da_cap, g_chunk_da, chunk_t);
	chunk_t* chunk = &g_chunk_da[g_chunk_da_len-1];
	chunk->top_left_tc.x = tc.x - cool_mod(tc.x, CHUNK_SIDE);
	chunk->top_left_tc.y = tc.y - cool_mod(tc.y, CHUNK_SIDE);
	chunk->tiles = calloc(CHUNK_SIDE * CHUNK_SIDE, sizeof(tile_t));
	for (int i = 0; i < CHUNK_SIDE * CHUNK_SIDE; i++)
	{
		chunk->tiles[i].floor = FLOOR_GRASSLAND;
		if (rand() % 3 == 0)
		{
			chunk->tiles[i].obj = (obj_t){.type = OBJ_TREE};
		}
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
	tc_t bottom_right_tc = sc_to_tc((sc_t){WINDOW_SIDE-1, WINDOW_SIDE-1});
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

		if (rand() % 7 == 1)
		{
			tile->floor = FLOOR_DESERT;
		}

		tile->obj = (obj_t){0};
		if (tile->floor != FLOOR_WATER)
		{
			if (rand() % 21 == 1)
			{
				tile->obj.type = OBJ_ENEMY_FLY;
			}
			else if (rand() % 13 == 3)
			{
				tile->obj.type = OBJ_ROCK;
			}
			else if (rand() % 13 == 3)
			{
				tile->obj.type = OBJ_TREE;
			}
		}
	}

	/* Place the crystal somewhere near the middle of the map. */
	g_crystal_tc = (tc_t){
		rand() % 5 - 2,
		rand() % 5 - 2};
	map_tile(g_crystal_tc)->obj = (obj_t){.type = OBJ_CRYSTAL};

	/* Place the 3 units around the crystal. */
	tc_t tc;
	tc = g_crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) += 1;
	map_tile(tc)->obj = (obj_t){.type = OBJ_UNIT_RED};
	tc = g_crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) -= 1;
	map_tile(tc)->obj = (obj_t){.type = OBJ_UNIT_BLUE};
	tc = g_crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) += rand() % 2 ? 2 : -2;
	map_tile(tc)->obj = (obj_t){.type = OBJ_UNIT_PINK};

	/* Also place a tower near the crystal. */
	tc = g_crystal_tc; tc.x += rand() % 2 ? 1 : -1; tc.y += rand() % 2 ? 1 : -1;
	map_tile(tc)->obj = (obj_t){.type = OBJ_TOWER, .ammo = 2};

	/* Making sure that enemies close to the crystal are eggs so that the player
	 * has some time to take action against them. */
	for (tc_iter_rect_t it = tc_iter_rect_init(tc_radius_to_rect(g_crystal_tc, 3));
		tc_iter_rect_cond(&it); tc_iter_rect_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (tile->obj.type == OBJ_ENEMY_FLY)
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
}

void handle_mouse_motion(sc_t sc)
{
	/* Update the hovered tile. */
	map_clear_hovered();
	tile_t* tile = map_tile(sc_to_tc(sc));
	if (tile != NULL)
	{
		tile->is_hovered = true;
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
		case OBJ_UNIT_RED:
		case OBJ_UNIT_BLUE:
		case OBJ_UNIT_PINK:
			return true;
		default:
			return false;
	}
}

void handle_mouse_click(bool is_left_click)
{
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

	if (new_selected->options[OPTION_WALK] && is_left_click)
	{
		/* A unit is moving. */
		g_motion.t = 0;
		g_motion.t_max = 7;
		g_motion.src = map_tile_tc(old_selected);
		g_motion.dst = map_tile_tc(new_selected);
		g_motion.obj = old_selected->obj;
		g_motion.obj.can_still_act = false;
		old_selected->obj = (obj_t){0};
		map_clear_options();
		return;
	}
	else if (new_selected->options[OPTION_TOWER] && !is_left_click)
	{
		/* A unit is placing a tower. */
		g_motion.t = 0;
		g_motion.t_max = 7;
		g_motion.src = map_tile_tc(old_selected);
		g_motion.dst = map_tile_tc(new_selected);
		g_motion.obj = (obj_t){.type = OBJ_TOWER, .ammo = 4};
		g_tower_available = false;
		old_selected->obj.can_still_act = false;
		map_clear_options();
		return;
	}

	if (is_left_click)
	{
		map_clear_options();
	}
	if (obj_type_is_unit(new_selected->obj.type) &&
		new_selected->obj.can_still_act &&
		is_left_click)
	{
		int walk_dist = 
			new_selected->obj.type == OBJ_UNIT_RED ? 4 :
			new_selected->obj.type == OBJ_UNIT_BLUE ? 3 :
			new_selected->obj.type == OBJ_UNIT_PINK ? 2 :
			(assert(false), 0);
		int tower_dist =
			new_selected->obj.type == OBJ_UNIT_RED ? 1 :
			new_selected->obj.type == OBJ_UNIT_BLUE ? 2 :
			new_selected->obj.type == OBJ_UNIT_PINK ? 3 :
			(assert(false), 0);
		/* A unit that can still act is selected, accessible tiles
		 * have to be flagged as walkable. */
		tc_t const selected_tc = map_tile_tc(new_selected);
		int const radius = max(walk_dist, tower_dist);
		for (tc_iter_rect_t it = tc_iter_rect_init(tc_radius_to_rect(selected_tc, radius));
			tc_iter_rect_cond(&it); tc_iter_rect_next(&it))
		{
			if (tc_eq(selected_tc, it.tc))
			{
				continue;
			}
			map_tile(it.tc)->options[OPTION_WALK] =
				path_exists(selected_tc, it.tc, walk_dist);
			map_tile(it.tc)->options[OPTION_TOWER] =
				g_tower_available &&
				path_exists(selected_tc, it.tc, tower_dist);
		}
	}
}

void handle_mouse_wheel(int wheel_motion)
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
		g_tile_side_pixels += wheel_motion;
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
			if (wheel_motion < 0)
			{
				g_tile_side_pixels -= g_tile_side_pixels % 16;
				wheel_motion++;
			}
			else if (wheel_motion > 0)
			{
				g_tile_side_pixels -= g_tile_side_pixels % 16;
				g_tile_side_pixels += 16;
				wheel_motion--;
			}
		}
		g_tile_side_pixels += wheel_motion * 16;
		if (g_tile_side_pixels < 16)
		{
			g_tile_side_pixels = 16;
		}
		else if (g_tile_side_pixels > WINDOW_SIDE / 2)
		{
			g_tile_side_pixels = (WINDOW_SIDE / 2) - (WINDOW_SIDE / 2) % 16;
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
				src_tile->obj.can_still_act = false;
				if (rand() % 4 == 0)
				{
					src_tile->obj.type = OBJ_ENEMY_FLY;
				}
				return false;
			}
			else if (src_tile->obj.type == OBJ_ENEMY_FLY)
			{
				/* A fly can move or lay an egg to an adjacent tile,
				 * which is to be decided. */
				tc_t dst_tc = src_tc;

				if ((rand() >> 5) % 6 == 0)
				{
					/* The destination can be random. */
					if ((rand() >> 5) % 2)
					{
						dst_tc.x += rand() % 2 ? 1 : -1;
					}
					else
					{
						dst_tc.y += rand() % 2 ? 1 : -1;
					}
				}
				else
				{
					/* Does the dst tile is differect from the current fly tile along the X axis?
					 * This is random choice, except when one option does not make sense. */
					bool axis_x = rand() % 2 == 0;
					if (src_tc.y == g_crystal_tc.y)
					{
						axis_x = true;
					}
					else if (src_tc.x == g_crystal_tc.x)
					{
						axis_x = false;
					}
					if (axis_x)
					{
						dst_tc.x += src_tc.x < g_crystal_tc.x ? 1 : -1;
					}
					else
					{
						dst_tc.y += src_tc.y < g_crystal_tc.y ? 1 : -1;
					}
				}

				if (!tc_in_map(dst_tc) || tc_eq(src_tc, dst_tc))
				{
					src_tile->obj.can_still_act = false;
					return false;
				}
				tile_t* dst_tile = map_tile(dst_tc);

				bool can_walk_on =
					dst_tile->obj.type == OBJ_NONE ||
					dst_tile->obj.type == OBJ_CRYSTAL ||
					obj_type_is_unit(dst_tile->obj.type) ||
					dst_tile->obj.type == OBJ_TOWER ||
					dst_tile->obj.type == OBJ_TOWER_OFF ||
					dst_tile->obj.type == OBJ_TREE;
				if (!can_walk_on)
				{
					src_tile->obj.can_still_act = false;
					return false;
				}

				bool lay_egg = rand() % 11 == 0;

				g_motion.t = 0;
				g_motion.t_max = 6;
				g_motion.src = src_tc;
				g_motion.dst = dst_tc;
				src_tile->obj.can_still_act = false;
				if (lay_egg)
				{
					g_motion.obj = (obj_t){.type = OBJ_ENEMY_EGG};
				}
				else
				{
					g_motion.obj = src_tile->obj;
					src_tile->obj = (obj_t){0};
				}
				return false;
			}
			else if (src_tile->obj.type == OBJ_ENEMY_BIG)
			{
				/* A big enemy will first shoot eggs at the towers it can see
				 * before moving or laying an egg nearby. */

				struct dir_t
				{
					int dx, dy;
					tc_t tc;
					bool is_valid;
				};
				typedef struct dir_t dir_t;
				dir_t dirs[4] = {
					{.dx = 1, .dy = 0},
					{.dx = -1, .dy = 0},
					{.dx = 0, .dy = 1},
					{.dx = 0, .dy = -1}};
				for (int i = 0; i < 4; i ++)
				{
					dirs[i].tc = src_tc;
					dirs[i].is_valid = tc_in_map(dirs[i].tc);
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
						if (type == OBJ_TOWER || type == OBJ_TOWER_OFF)
						{
							/* Shoot an egg that will destroy a tower
							 * (and the big enemy can still act). */
							g_motion.t = 0;
							g_motion.t_max = 14;
							g_motion.src = src_tc;
							g_motion.dst = dirs[i].tc;
							g_motion.obj = (obj_t){.type = OBJ_ENEMY_EGG};
							return false;
						}
						if (type != OBJ_NONE)
						{
							dirs[i].is_valid = false;
						}
					}
				}

				/* Now it will move or lay an egg.
				 * Note that it can do that on a distance of two tiles. */
				tc_t dst_tc = src_tc;

				if ((rand() >> 5) % 6 == 0)
				{
					/* The destination can be random. */
					if ((rand() >> 5) % 2)
					{
						dst_tc.x += (rand() % 2 ? 1 : -1) * (rand() % 2 ? 1 : 2);
					}
					else
					{
						dst_tc.y += (rand() % 2 ? 1 : -1) * (rand() % 2 ? 1 : 2);
					}
				}
				else
				{
					/* Does the dst tile is differect from the current fly tile along the X axis?
					 * This is random choice, except when one option does not make sense. */
					bool axis_x = rand() % 2 == 0;
					if (src_tc.y == g_crystal_tc.y)
					{
						axis_x = true;
					}
					else if (src_tc.x == g_crystal_tc.x)
					{
						axis_x = false;
					}
					if (axis_x)
					{
						dst_tc.x +=
							(src_tc.x < g_crystal_tc.x ? 1 : -1) *
							(abs(src_tc.x - g_crystal_tc.x) == 1 || rand() % 2 ? 1 : 2);
					}
					else
					{
						dst_tc.y +=
							(src_tc.y < g_crystal_tc.y ? 1 : -1) *
							(abs(src_tc.y - g_crystal_tc.y) == 1 || rand() % 2 ? 1 : 2);
					}
				}

				if (!tc_in_map(dst_tc) || tc_eq(src_tc, dst_tc))
				{
					src_tile->obj.can_still_act = false;
					return false;
				}
				tile_t* dst_tile = map_tile(dst_tc);

				bool can_walk_on =
					dst_tile->obj.type == OBJ_NONE ||
					dst_tile->obj.type == OBJ_CRYSTAL ||
					obj_type_is_unit(dst_tile->obj.type) ||
					dst_tile->obj.type == OBJ_TOWER ||
					dst_tile->obj.type == OBJ_TOWER_OFF ||
					dst_tile->obj.type == OBJ_TREE;
				if (!can_walk_on)
				{
					src_tile->obj.can_still_act = false;
					return false;
				}

				bool lay_egg = rand() % 11 == 0;

				g_motion.t = 0;
				g_motion.t_max = 6;
				g_motion.src = src_tc;
				g_motion.dst = dst_tc;
				src_tile->obj.can_still_act = false;
				if (lay_egg)
				{
					g_motion.obj = (obj_t){.type = OBJ_ENEMY_EGG};
				}
				else
				{
					g_motion.obj = src_tile->obj;
					src_tile->obj = (obj_t){0};
				}
				return false;
			}
			else
			{
				assert(false);
			}
		}
	}

	/* Spawn enemy flies in the top corners. */
	if ((rand() >> 3) % 2 == 0 &&
		map_tile((tc_t){-8, 4})->obj.type != OBJ_ENEMY_FLY)
	{
		g_motion.t = 0;
		g_motion.t_max = 6;
		g_motion.src = (tc_t){-9, 4};
		g_motion.dst = (tc_t){-8, 4};
		g_motion.obj = (obj_t){.type = OBJ_ENEMY_FLY};
		g_motion.obj.can_still_act = false;
		return false;
	}
	if ((rand() >> 3) % 2 == 0 &&
		map_tile((tc_t){8, -4})->obj.type != OBJ_ENEMY_FLY)
	{
		g_motion.t = 0;
		g_motion.t_max = 6;
		g_motion.src = (tc_t){9, -4};
		g_motion.dst = (tc_t){8, -4};
		g_motion.obj = (obj_t){.type = OBJ_ENEMY_FLY};
		g_motion.obj.can_still_act = false;
		return false;
	}

	/* Spawn big enemies in the bottom corners. */
	if (g_turn > 10 && g_turn % 7 == 0 &&
		map_tile((tc_t){0, 9})->obj.type != OBJ_ENEMY_BIG)
	{
		g_motion.t = 0;
		g_motion.t_max = 6;
		g_motion.src = (tc_t){0, 10};
		g_motion.dst = (tc_t){0, 9};
		g_motion.obj = (obj_t){.type = OBJ_ENEMY_BIG};
		g_motion.obj.can_still_act = false;
		return false;
	}
	if (g_turn > 10 && g_turn % 7 == 1 &&
		map_tile((tc_t){0, -9})->obj.type != OBJ_ENEMY_BIG)
	{
		g_motion.t = 0;
		g_motion.t_max = 6;
		g_motion.src = (tc_t){0, -10};
		g_motion.dst = (tc_t){0, -9};
		g_motion.obj = (obj_t){.type = OBJ_ENEMY_BIG};
		g_motion.obj.can_still_act = false;
		return false;
	}

	return true;
}

void tower_shoot(tc_t tower_tc, tc_t target_tc)
{
	g_motion.t = 0;
	g_motion.t_max = 12;
	g_motion.src = tower_tc;
	g_motion.dst = target_tc;
	g_motion.obj = (obj_t){.type = OBJ_SHOT};

	/* Update the tower. */
	tile_t* tower_tile = map_tile(tower_tc);
	tower_tile->obj.can_still_act = false;
	tower_tile->obj.ammo--;
	if (tower_tile->obj.ammo <= 0)
	{
		tower_tile->obj.type = OBJ_TOWER_OFF;
	}
}

bool game_play_towers(void)
{
	for (tc_iter_all_spiral_t it = tc_iter_all_spiral_init(g_crystal_tc);
		tc_iter_all_spiral_cond(&it); tc_iter_all_spiral_next(&it))
	{
		tc_t src_tc = it.tc;
		tile_t* tower_tile = map_tile(src_tc);
		if (tower_tile->obj.can_still_act)
		{
			assert(tower_tile->obj.type == OBJ_TOWER);

			struct dir_t
			{
				int dx, dy;
				tc_t tc;
				bool is_valid;
			};
			typedef struct dir_t dir_t;
			dir_t dirs[4] = {
				{.dx = 1, .dy = 0},
				{.dx = -1, .dy = 0},
				{.dx = 0, .dy = 1},
				{.dx = 0, .dy = -1}};
			for (int i = 0; i < 4; i ++)
			{
				/* Note that a tower cannot shoot at a distance of one tile. */
				dirs[i].tc.x = src_tc.x + dirs[i].dx;
				dirs[i].tc.y = src_tc.y + dirs[i].dy;
				dirs[i].is_valid =
					tc_in_map(dirs[i].tc) &&
					map_tile(dirs[i].tc)->obj.type == OBJ_NONE;
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
					if (type == OBJ_ENEMY_FLY ||
						type == OBJ_ENEMY_BIG ||
						type == OBJ_ENEMY_EGG)
					{
						tower_shoot(src_tc, dirs[i].tc);
						return false;
					}
					if (type != OBJ_NONE)
					{
						dirs[i].is_valid = false;
					}
				}
			}

			tower_tile->obj.can_still_act = false;
			return false;
		}
	}

	return true;
}

void start_enemy_phase(void)
{
	g_phase = PHASE_ENEMY;
	g_phase_time = 0;
	map_clear_options();
	map_clear_selected();
	map_clear_can_still_act();

	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (tile->obj.type == OBJ_ENEMY_FLY ||
			tile->obj.type == OBJ_ENEMY_BIG ||
			tile->obj.type == OBJ_ENEMY_EGG)
		{
			tile->obj.can_still_act = true;
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

	for (tc_iter_all_t it = tc_iter_all_init();
		tc_iter_all_cond(&it); tc_iter_all_next(&it))
	{
		tile_t* tile = map_tile(it.tc);
		if (tile->obj.type == OBJ_TOWER)
		{
			tile->obj.can_still_act = true;
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
			if (g_motion.obj.type == OBJ_SHOT)
			{
				dst_tile->obj = (obj_t){0};
			}
			else
			{
				dst_tile->obj = g_motion.obj;
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
	g_tc_sc_offset.x -= sc.x + g_tile_side_pixels / 2 - WINDOW_SIDE / 2;
	g_tc_sc_offset.y -= sc.y + g_tile_side_pixels / 2 - WINDOW_SIDE / 2;
}

int main(void)
{
	printf("Why Crystals ? version 0.0.1 indev\n");

	srand(time(NULL));

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		assert(false);
	}

	g_window = SDL_CreateWindow("Why Crystals ?",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIDE, WINDOW_SIDE, 0);
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
	
	g_motion = (motion_t){0};
	map_generate();
	center_view(g_crystal_tc);

	g_turn = 0;
	g_game_over = false;
	start_player_phase();

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
						case SDLK_SPACE:
							if (g_phase == PHASE_PLAYER)
							{
								start_enemy_phase();
							}
						break;
						case SDLK_c:
							center_view(g_crystal_tc);
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
						start_enemy_phase();
					}
					if (g_phase == PHASE_PLAYER)
					{
						handle_mouse_click(event.button.button == SDL_BUTTON_LEFT);
					}
				break;
			}
		}

		game_perform();

		SDL_SetRenderDrawColor(g_renderer, 30, 40, 80, 255);
		SDL_RenderClear(g_renderer);
		
		draw_map();

		char string[60];
		sprintf(string, "TURN %d", g_turn);
		draw_text(string, (rgb_t){0, 0, 0}, (sc_t){0, 0}, false);
		if (g_game_over)
		{
			draw_text("GAME OVER", (rgb_t){0, 0, 0}, (sc_t){0, 20}, false);
		}
		
		SDL_RenderPresent(g_renderer);
	}

	if (!g_game_over)
	{
		printf("Quit on turn %d\n", g_turn);
	}

	TTF_Quit();
	IMG_Quit(); 
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit(); 
	return 0;
}
