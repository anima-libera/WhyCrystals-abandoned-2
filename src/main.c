
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

/* Tile coordinates. */
struct tc_t
{
	int x, y;
};
typedef struct tc_t tc_t;

int g_map_side;

void tc_iter(tc_t* tc)
{
	tc->x++;
	if (tc->x >= g_map_side)
	{
		tc->x = 0;
		tc->y++;
	}
}

bool tc_in_map(tc_t tc)
{
	return 0 <= tc.x && tc.x < g_map_side && 0 <= tc.y && tc.y < g_map_side;
}

bool tc_eq(tc_t tc_a, tc_t tc_b)
{
	return tc_a.x == tc_b.x && tc_a.y == tc_b.y;
}

/* Screen coordinates. */
struct sc_t
{
	int x, y;
};
typedef struct sc_t sc_t;

#define TILE_SIDE_PIXELS 64
#define WINDOW_SIDE (800 - 800 % TILE_SIDE_PIXELS)

sc_t tc_to_sc(tc_t tc)
{
	return (sc_t){tc.x * TILE_SIDE_PIXELS, tc.y * TILE_SIDE_PIXELS};
}

tc_t sc_to_tc(sc_t sc)
{
	return (tc_t){sc.x / TILE_SIDE_PIXELS, sc.y / TILE_SIDE_PIXELS};
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
};
typedef enum floor_type_t floor_type_t;

struct tile_t
{
	floor_type_t floor;
	obj_t obj;
	bool is_hovered;
	bool is_selected;
	bool is_available;
};
typedef struct tile_t tile_t;

void draw_tile(tile_t const* tile, sc_t sc, int side)
{
	/* Draw the floor tile sprite. */
	sprite_t sprite;
	switch (tile->floor)
	{
		case FLOOR_GRASSLAND: sprite = SPRITE_GRASSLAND; break;
		case FLOOR_DESERT:    sprite = SPRITE_DESERT;    break;
		default: assert(false);
	}
	SDL_Rect rect = {.x = sc.x, .y = sc.y, .w = side, .h = side};
	draw_sprite(sprite, &rect);

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
	}
	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(g_renderer, &rect);
	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

	/* Draw additional UI square around the tile. */
	if (tile->is_selected)
	{
		SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(g_renderer, &rect);
	}
	else if (tile->is_hovered && !tile->is_available)
	{
		SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(g_renderer, &rect);
	}
	if (tile->is_available)
	{
		int const margin = tile->is_hovered ? 3 : 5;
		rect.x += margin;
		rect.y += margin;
		rect.w -= margin * 2;
		rect.h -= margin * 2;
		SDL_SetRenderDrawColor(g_renderer, 0, 100, 0, 255);
		SDL_RenderDrawRect(g_renderer, &rect);
	}

	/* Draw the object that is on the tile. */
	draw_obj(&tile->obj, sc, side);

	/* Draw the tower ammo count. */
	if ((tile->is_selected || tile->is_hovered) && tile->obj.type == OBJ_TOWER)
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

tile_t* g_map_grid;
motion_t g_motion;

tile_t* map_tile(tc_t tc)
{
	if (tc_in_map(tc))
	{
		return &g_map_grid[tc.y * g_map_side + tc.x];
	}
	else
	{
		return NULL;
	}
}

void draw_map(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		draw_tile(map_tile(tc), tc_to_sc(tc), TILE_SIDE_PIXELS);
	}

	if (g_motion.t_max > 0)
	{
		motion_t* m = &g_motion;
		float r = (float)m->t / (float)m->t_max;
		float x = (float)m->src.x * (1.0f - r) + (float)m->dst.x * r;
		float y = (float)m->src.y * (1.0f - r) + (float)m->dst.y * r;
		draw_obj(&m->obj,
			(sc_t){x * (float)TILE_SIDE_PIXELS, y * (float)TILE_SIDE_PIXELS},
			TILE_SIDE_PIXELS);
	}
}

void map_generate(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		tile_t* tile = map_tile(tc);

		if (rand() % 7 == 1)
		{
			tile->floor = FLOOR_DESERT;
		}

		tile->obj = (obj_t){0};
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

	/* Place the crystal somewhere near the middle of the map. */
	tc_t const crystal_tc = {
		g_map_side / 2 + rand() % 5 - 2,
		g_map_side / 2 + rand() % 5 - 2};
	map_tile(crystal_tc)->obj = (obj_t){.type = OBJ_CRYSTAL};

	/* Place the 3 units around the crystal. */
	tc_t tc;
	tc = crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) += 1;
	map_tile(tc)->obj = (obj_t){.type = OBJ_UNIT_RED};
	tc = crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) -= 1;
	map_tile(tc)->obj = (obj_t){.type = OBJ_UNIT_RED};
	tc = crystal_tc; *(rand() % 2 ? &tc.x : &tc.y) += rand() % 2 ? 2 : -2;
	map_tile(tc)->obj = (obj_t){.type = OBJ_UNIT_RED};

	/* Also place a tower near the crystal. */
	tc = crystal_tc; tc.x += rand() % 2 ? 1 : -1; tc.y += rand() % 2 ? 1 : -1;
	map_tile(tc)->obj = (obj_t){.type = OBJ_TOWER, .ammo = 2};

	/* Making sure that enemies close to the crystal are eggs so that the player
	 * has some time to take action against them. */
	for (tc.y = crystal_tc.y - 2; tc.y < crystal_tc.y + 3; tc.y++)
	for (tc.x = crystal_tc.x - 2; tc.x < crystal_tc.x + 3; tc.x++)
	{
		tile_t* tile = map_tile(tc);
		if (tile->obj.type == OBJ_ENEMY_FLY)
		{
			tile->obj.type = OBJ_ENEMY_EGG;
		}
	}
}

void map_clear_available(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		map_tile(tc)->is_available = false;
	}
}

void map_clear_hovered(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		map_tile(tc)->is_hovered = false;
	}
}

void map_clear_selected(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		map_tile(tc)->is_selected = false;
	}
}

void map_clear_can_still_act(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		map_tile(tc)->obj.can_still_act = false;
	}
}

tile_t* map_hovered_tile(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		tile_t* tile = map_tile(tc);
		if (tile->is_hovered)
		{
			return tile;
		}
	}
	return NULL;
}

tile_t* map_selected_tile(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		tile_t* tile = map_tile(tc);
		if (tile->is_selected)
		{
			return tile;
		}
	}
	return NULL;
}

tile_t* map_crystal_tile(void)
{
	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		tile_t* tile = map_tile(tc);
		if (tile->obj.type == OBJ_CRYSTAL)
		{
			return tile;
		}
	}
	return NULL;
}

tc_t map_tile_tc(tile_t* tile)
{
	int offset = tile - g_map_grid;
	return (tc_t){
		.x = offset % g_map_side,
		.y = offset / g_map_side};
}

void handle_mouse_motion(sc_t sc)
{
	/* Update the hovered tile. */
	map_clear_hovered();
	map_tile(sc_to_tc(sc))->is_hovered = true;
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
/* The turn number, counting from zero. */
int g_turn;
/* A time counter that allows to put a delay inbetween successive actions
 * that can happen automatically a bit too fast. */
int g_phase_time;
/* Has the player lost the game? */
bool g_game_over;

void handle_mouse_click(bool is_left_click)
{
	tile_t* old_selected = map_selected_tile();

	/* Update the selected tile. */
	map_clear_selected();
	tile_t* new_selected = map_hovered_tile();
	new_selected->is_selected = true;

	/* If an animation is playing, then we forbid interaction. */
	if (g_motion.t_max > 0)
	{
		return;
	}

	if (new_selected->is_available)
	{
		/* A unit is moving or placing a tower. */
		assert(old_selected->obj.type == OBJ_UNIT_RED);
		g_motion.t = 0;
		g_motion.t_max = 7;
		g_motion.src = map_tile_tc(old_selected);
		g_motion.dst = map_tile_tc(new_selected);
		if (is_left_click)
		{
			/* Moving. */
			g_motion.obj = old_selected->obj;
			g_motion.obj.can_still_act = false;
			old_selected->obj = (obj_t){0};
		}
		else if (g_tower_available)
		{
			/* Placing a tower. */
			g_motion.obj = (obj_t){.type = OBJ_TOWER, .ammo = 4};
			g_tower_available = false;
			old_selected->obj.can_still_act = false;
		}
		map_clear_available();
		return;
	}
	map_clear_available();
	if (new_selected->obj.type == OBJ_UNIT_RED &&
		new_selected->obj.can_still_act)
	{
		/* A unit that can still act is selected, accessible tiles
		 * have to be flagged as available for action. */
		tc_t const selected_tc = map_tile_tc(new_selected);
		for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
		{
			int const dist = abs(tc.x - selected_tc.x) + abs(tc.y - selected_tc.y);
			bool is_accessible =
				dist != 0 && dist <= 2 &&
				map_tile(tc)->obj.type == OBJ_NONE;
			map_tile(tc)->is_available = is_accessible;
		}
	}
}

bool game_play_enemy(void)
{
	for (tc_t src_tc = {0}; tc_in_map(src_tc); tc_iter(&src_tc))
	{
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
					/* The destination can also be a step towards the crystal
					 * (and in case there is no crystal, the center of the map will do). */
					tc_t crystal_tc = {g_map_side / 2, g_map_side / 2};
					tile_t* crystal_tile = map_crystal_tile();
					if (crystal_tile != NULL)
					{
						crystal_tc = map_tile_tc(crystal_tile);
					}
					/* Does the dst tile is differect from the current fly tile along the X axis?
					 * This is random choice, except when one option does not make sense. */
					bool axis_x = rand() % 2 == 0;
					if (src_tc.y == crystal_tc.y)
					{
						axis_x = true;
					}
					else if (src_tc.x == crystal_tc.x)
					{
						axis_x = false;
					}
					if (axis_x)
					{
						dst_tc.x += src_tc.x < crystal_tc.x ? 1 : -1;
					}
					else
					{
						dst_tc.y += src_tc.y < crystal_tc.y ? 1 : -1;
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
					dst_tile->obj.type == OBJ_UNIT_RED ||
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
					/* The destination can also be a step towards the crystal
					 * (and in case there is no crystal, the center of the map will do). */
					tc_t crystal_tc = {g_map_side / 2, g_map_side / 2};
					tile_t* crystal_tile = map_crystal_tile();
					if (crystal_tile != NULL)
					{
						crystal_tc = map_tile_tc(crystal_tile);
					}
					/* Does the dst tile is differect from the current fly tile along the X axis?
					 * This is random choice, except when one option does not make sense. */
					bool axis_x = rand() % 2 == 0;
					if (src_tc.y == crystal_tc.y)
					{
						axis_x = true;
					}
					else if (src_tc.x == crystal_tc.x)
					{
						axis_x = false;
					}
					if (axis_x)
					{
						dst_tc.x +=
							(src_tc.x < crystal_tc.x ? 1 : -1) *
							(abs(src_tc.x - crystal_tc.x) == 1 || rand() % 2 ? 1 : 2);
					}
					else
					{
						dst_tc.y +=
							(src_tc.y < crystal_tc.y ? 1 : -1) *
							(abs(src_tc.y - crystal_tc.y) == 1 || rand() % 2 ? 1 : 2);
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
					dst_tile->obj.type == OBJ_UNIT_RED ||
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
		map_tile((tc_t){0, 0})->obj.type != OBJ_ENEMY_FLY)
	{
		g_motion.t = 0;
		g_motion.t_max = 6;
		g_motion.src = (tc_t){-1, -1};
		g_motion.dst = (tc_t){0, 0};
		g_motion.obj = (obj_t){.type = OBJ_ENEMY_FLY};
		g_motion.obj.can_still_act = false;
		return false;
	}
	if ((rand() >> 3) % 2 == 0 &&
		map_tile((tc_t){g_map_side-1, 0})->obj.type != OBJ_ENEMY_FLY)
	{
		g_motion.t = 0;
		g_motion.t_max = 6;
		g_motion.src = (tc_t){g_map_side, -1};
		g_motion.dst = (tc_t){g_map_side-1, 0};
		g_motion.obj = (obj_t){.type = OBJ_ENEMY_FLY};
		g_motion.obj.can_still_act = false;
		return false;
	}

	/* Spawn big enemies in the bottom corners. */
	if (g_turn > 10 && g_turn % 7 == 0 &&
		map_tile((tc_t){0, g_map_side-1})->obj.type != OBJ_ENEMY_BIG)
	{
		g_motion.t = 0;
		g_motion.t_max = 6;
		g_motion.src = (tc_t){-1, g_map_side};
		g_motion.dst = (tc_t){0, g_map_side-1};
		g_motion.obj = (obj_t){.type = OBJ_ENEMY_BIG};
		g_motion.obj.can_still_act = false;
		return false;
	}
	if (g_turn > 10 && g_turn % 7 == 1 &&
		map_tile((tc_t){g_map_side-1, g_map_side-1})->obj.type != OBJ_ENEMY_BIG)
	{
		g_motion.t = 0;
		g_motion.t_max = 6;
		g_motion.src = (tc_t){g_map_side, g_map_side};
		g_motion.dst = (tc_t){g_map_side-1, g_map_side-1};
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
	for (tc_t src_tc = {0}; tc_in_map(src_tc); tc_iter(&src_tc))
	{
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
	map_clear_available();
	map_clear_selected();
	map_clear_can_still_act();

	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		tile_t* tile = map_tile(tc);
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
	map_clear_available();
	map_clear_selected();
	map_clear_can_still_act();

	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		tile_t* tile = map_tile(tc);
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

	for (tc_t tc = {0}; tc_in_map(tc); tc_iter(&tc))
	{
		tile_t* tile = map_tile(tc);
		if (tile->obj.type == OBJ_UNIT_RED)
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

int main(void)
{
	srand(time(NULL));

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		assert(false);
	}

	g_window = SDL_CreateWindow("Pyea",
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
	
	g_map_side = WINDOW_SIDE / TILE_SIDE_PIXELS;
	g_map_grid = calloc(g_map_side * g_map_side, sizeof(tile_t));
	g_motion = (motion_t){0};
	map_generate();

	g_tower_available = true;
	g_turn = 0;
	g_phase_time = 0;
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
						case SDLK_RETURN:
						case SDLK_SPACE:
							if (g_phase == PHASE_PLAYER)
							{
								start_enemy_phase();
							}
						break;
					}
				break;
				case SDL_MOUSEMOTION:
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
