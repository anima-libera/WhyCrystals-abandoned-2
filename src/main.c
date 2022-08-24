
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "embedded.h"
#include "rendering.h"
#include "sprites.h"

#define CELL_SIDE_PIXELS 64
#define WINDOW_SIDE (800 - 800 % CELL_SIDE_PIXELS)

void draw_text(char const* text, int r, int g, int b, int x, int y, bool center_x)
{
	SDL_Color color = {.r = r, .g = g, .b = b, .a = 255};
	SDL_Surface* surface = TTF_RenderText_Solid(g_font, text, color);
	assert(surface != NULL);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(texture != NULL);
	SDL_FreeSurface(surface);
	SDL_Rect rect = {.y = y};
	SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
	rect.x = center_x ? x-rect.w/2 : x;
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

void draw_obj(obj_t const* obj, int x, int y, int side)
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
	SDL_Rect rect = {.x = x, .y = y, .w = side, .h = side};
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
	bool is_available;
	bool is_hovered;
	bool is_selected;
};
typedef struct tile_t tile_t;

void draw_cell(tile_t const* tile, int x, int y, int side)
{
	/* Draw the floor tile sprite. */
	sprite_t sprite;
	switch (tile->floor)
	{
		case FLOOR_GRASSLAND: sprite = SPRITE_GRASSLAND; break;
		case FLOOR_DESERT:    sprite = SPRITE_DESERT;    break;
		default: assert(false);
	}
	SDL_Rect rect = {.x = x, .y = y, .w = side, .h = side};
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
	draw_obj(&tile->obj, x, y, side);

	/* Draw the tower ammo count. */
	if ((tile->is_selected || tile->is_hovered) && tile->obj.type == OBJ_TOWER)
	{
		char string[20];
		sprintf(string, "%d", tile->obj.ammo);
		draw_text(string, 0, 0, 0, rect.x + 3, rect.y - 1, false);
	}
}

struct motion_t
{
	int t;
	int t_max;
	int src_x, src_y;
	int dst_x, dst_y;
	obj_t obj;
};
typedef struct motion_t motion_t;

struct map_t
{
	int grid_side;
	tile_t* grid;
	motion_t motion;
};
typedef struct map_t map_t;

tile_t* map_tile(map_t* map, int x, int y)
{
	return &map->grid[y * map->grid_side + x];
}

void draw_map(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		draw_cell(map_tile(map, x, y),
			x * CELL_SIDE_PIXELS, y * CELL_SIDE_PIXELS, CELL_SIDE_PIXELS);
	}

	if (map->motion.t_max > 0)
	{
		motion_t* m = &map->motion;
		float r = (float)m->t / (float)m->t_max;
		float x = (float)m->src_x * (1.0f - r) + (float)m->dst_x * r;
		float y = (float)m->src_y * (1.0f - r) + (float)m->dst_y * r;
		draw_obj(&m->obj,
			x * (float)CELL_SIDE_PIXELS, y * (float)CELL_SIDE_PIXELS,
			CELL_SIDE_PIXELS);
	}
}

void map_generate(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_tile(map, x, y)->obj = (obj_t){0};

		if (rand() % 7 == 1)
		{
			map_tile(map, x, y)->floor = FLOOR_DESERT;
		}

		if (rand() % 21 == 1)
		{
			map_tile(map, x, y)->obj.type = OBJ_ENEMY_FLY;
		}
		else if (rand() % 13 == 3)
		{
			map_tile(map, x, y)->obj.type = OBJ_ROCK;
		}
		else if (rand() % 13 == 3)
		{
			map_tile(map, x, y)->obj.type = OBJ_TREE;
		}
	}

	int cx = map->grid_side / 2 + rand() % 5 - 2;
	int cy = map->grid_side / 2 + rand() % 5 - 2;
	map_tile(map, cx, cy)->obj.type = OBJ_CRYSTAL;

	int ux, uy;
	ux = cx; uy = cy; *(rand() % 2 ? &ux : &uy) += 1;
	map_tile(map, ux, uy)->obj.type = OBJ_UNIT_RED;
	ux = cx; uy = cy; *(rand() % 2 ? &ux : &uy) -= 1;
	map_tile(map, ux, uy)->obj.type = OBJ_UNIT_RED;
	ux = cx; uy = cy; *(rand() % 2 ? &ux : &uy) += rand() % 2 ? 2 : -2;
	map_tile(map, ux, uy)->obj.type = OBJ_UNIT_RED;

	ux = cx; uy = cy; ux += rand() % 2 ? 1 : -1; uy += rand() % 2 ? 1 : -1;
	map_tile(map, ux, uy)->obj.type = OBJ_TOWER;
	map_tile(map, ux, uy)->obj.ammo = 2;

	for (int y = cy - 2; y < cy + 3; y++)
	for (int x = cx - 2; x < cx + 3; x++)
	{
		tile_t* tile = map_tile(map, x, y);
		if (tile->obj.type == OBJ_ENEMY_FLY)
		{
			tile->obj.type = OBJ_ENEMY_EGG;
		}
	}
}

void map_clear_green(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_tile(map, x, y)->is_available = false;
	}
}

void map_clear_hovered(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_tile(map, x, y)->is_hovered = false;
	}
}

void map_clear_selected(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_tile(map, x, y)->is_selected = false;
	}
}

void map_clear_can_still_act(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_tile(map, x, y)->obj.can_still_act = false;
	}
}

tile_t* map_cell_from_pixel(map_t* map, int pixel_x, int pixel_y)
{
	return map_tile(map,
		pixel_x / CELL_SIDE_PIXELS, pixel_y / CELL_SIDE_PIXELS);
}

tile_t* map_hovered_cell(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		tile_t* tile = map_tile(map, x, y);
		if (tile->is_hovered)
		{
			return tile;
		}
	}
	return NULL;
}

tile_t* map_selected_cell(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		tile_t* tile = map_tile(map, x, y);
		if (tile->is_selected)
		{
			return tile;
		}
	}
	return NULL;
}

tile_t* map_crystal_cell(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		tile_t* tile = map_tile(map, x, y);
		if (tile->obj.type == OBJ_CRYSTAL)
		{
			return tile;
		}
	}
	return NULL;
}

void map_cell_coords(map_t* map, tile_t* tile, int* out_x, int* out_y)
{
	int cell_offset = tile - map->grid;
	if (out_x != NULL)
	{
		*out_x = cell_offset % map->grid_side;
	}
	if (out_y != NULL)
	{
		*out_y = cell_offset / map->grid_side;
	}
}

void handle_mouse_motion(map_t* map, int new_x, int new_y)
{
	map_clear_hovered(map);
	tile_t* tile = map_cell_from_pixel(map, new_x, new_y);
	tile->is_hovered = true;
}

struct game_state_t
{
	bool tower_available;
	bool player_phase;
	bool tower_phase;
	int t;
	bool game_is_lost;
	int turn;
};
typedef struct game_state_t game_state_t;

void handle_mouse_click(map_t* map, game_state_t* gs, bool is_left_click)
{
	tile_t* old_selected = map_selected_cell(map);
	map_clear_selected(map);
	tile_t* new_selected = map_hovered_cell(map);
	new_selected->is_selected = true;
	if (map->motion.t_max > 0)
	{
		return;
	}
	if (new_selected->is_available)
	{
		assert(old_selected->obj.type == OBJ_UNIT_RED);
		map->motion.t = 0;
		map->motion.t_max = 7;
		map_cell_coords(map, old_selected, &map->motion.src_x, &map->motion.src_y);
		map_cell_coords(map, new_selected, &map->motion.dst_x, &map->motion.dst_y);
		if (is_left_click)
		{
			map->motion.obj = old_selected->obj;
			map->motion.obj.can_still_act = false;
			old_selected->obj = (obj_t){0};
		}
		else if (gs->tower_available)
		{
			map->motion.obj = (obj_t){.type = OBJ_TOWER, .ammo = 4};
			gs->tower_available = false;
			old_selected->obj.can_still_act = false;
		}
		map_clear_green(map);
		return;
	}
	map_clear_green(map);
	if (new_selected->obj.type == OBJ_UNIT_RED &&
		new_selected->obj.can_still_act)
	{
		int selected_x, selected_y;
		map_cell_coords(map, new_selected, &selected_x, &selected_y);
		for (int y = 0; y < map->grid_side; y++)
		for (int x = 0; x < map->grid_side; x++)
		{
			int const dist = abs(x - selected_x) + abs(y - selected_y);
			bool is_accessible = dist != 0 && dist <= 2;
			if (map_tile(map, x, y)->obj.type != OBJ_NONE)
			{
				is_accessible = false;
			}
			map_tile(map, x, y)->is_available = is_accessible;
		}
	}
}

bool game_play_enemy(map_t* map, game_state_t* gs)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		tile_t* src_cell = map_tile(map, x, y);
		if (src_cell->obj.can_still_act)
		{
			if (src_cell->obj.type == OBJ_ENEMY_EGG)
			{
				src_cell->obj.can_still_act = false;
				if (rand() % 4 == 0)
				{
					src_cell->obj.type = OBJ_ENEMY_FLY;
				}
				return false;
			}
			else if (src_cell->obj.type == OBJ_ENEMY_FLY)
			{
				int dst_x = x, dst_y = y;

				if ((rand() >> 5) % 5 == 4)
				{
					if ((rand() >> 5) % 2)
					{
						dst_x += rand() % 2 ? 1 : -1;
					}
					else
					{
						dst_y += rand() % 2 ? 1 : -1;
					}
				}
				else
				{
					int crystal_x = map->grid_side / 2;
					int crystal_y = map->grid_side / 2;
					tile_t* crystal_cell = map_crystal_cell(map);
					if (crystal_cell != NULL)
					{
						map_cell_coords(map, crystal_cell, &crystal_x, &crystal_y);
					}
					bool move_x = rand() % 2 == 0 && x != crystal_x;
					if (move_x)
					{
						dst_x += x < crystal_x ? 1 : -1;
					}
					else
					{
						dst_y += y < crystal_y ? 1 : -1;
					}
				}

				if (dst_x < 0)
				{
					dst_x = 0;
				}
				else if (dst_x >= map->grid_side)
				{
					dst_x = map->grid_side - 1;
				}
				if (dst_y < 0)
				{
					dst_y = 0;
				}
				else if (dst_y >= map->grid_side)
				{
					dst_y = map->grid_side - 1;
				}

				tile_t* dst_cell = map_tile(map, dst_x, dst_y);
				if (dst_cell->obj.type != OBJ_CRYSTAL &&
					dst_cell->obj.type != OBJ_UNIT_RED &&
					dst_cell->obj.type != OBJ_TOWER &&
					dst_cell->obj.type != OBJ_TOWER_OFF &&
					dst_cell->obj.type != OBJ_TREE &&
					(dst_cell == src_cell || dst_cell->obj.type != OBJ_NONE))
				{
					src_cell->obj.can_still_act = false;
					return false;
				}

				bool lay_egg = rand() % 11 == 0;

				map->motion.t = 0;
				map->motion.t_max = 6;
				map_cell_coords(map, src_cell, &map->motion.src_x, &map->motion.src_y);
				map_cell_coords(map, dst_cell, &map->motion.dst_x, &map->motion.dst_y);
				src_cell->obj.can_still_act = false;
				if (lay_egg)
				{
					map->motion.obj = (obj_t){.type = OBJ_ENEMY_EGG};
				}
				else
				{
					map->motion.obj = src_cell->obj;
					src_cell->obj = (obj_t){0};
				}
				return false;
			}
			else if (src_cell->obj.type == OBJ_ENEMY_BIG)
			{
				struct dir_t
				{
					int dx, dy;
					int x, y;
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
					dirs[i].x = x;
					dirs[i].y = y;
					dirs[i].is_valid =
						dirs[i].x >= 0 && dirs[i].x < map->grid_side &&
						dirs[i].y >= 0 && dirs[i].y < map->grid_side;
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

						dirs[i].x += dirs[i].dx;
						dirs[i].y += dirs[i].dy;
						if (!(dirs[i].x >= 0 && dirs[i].x < map->grid_side &&
							dirs[i].y >= 0 && dirs[i].y < map->grid_side))
						{
							dirs[i].is_valid = false;
							continue;
						}
						obj_type_t type = map_tile(map, dirs[i].x, dirs[i].y)->obj.type;
						if (type == OBJ_TOWER || type == OBJ_TOWER_OFF)
						{
							map->motion.t = 0;
							map->motion.t_max = 14;
							map->motion.src_x = x;
							map->motion.src_y = y;
							map->motion.dst_x = dirs[i].x;
							map->motion.dst_y = dirs[i].y;
							map->motion.obj = (obj_t){.type = OBJ_ENEMY_EGG};
							return false;
						}
						if (type != OBJ_NONE)
						{
							dirs[i].is_valid = false;
						}
					}
				}

				int dst_x = x, dst_y = y;

				if ((rand() >> 5) % 5 == 4)
				{
					if ((rand() >> 5) % 2)
					{
						dst_x += (rand() % 2 ? 1 : -1) * (rand() % 2 ? 1 : 2);
					}
					else
					{
						dst_y += (rand() % 2 ? 1 : -1) * (rand() % 2 ? 1 : 2);
					}
				}
				else
				{
					int crystal_x = map->grid_side / 2;
					int crystal_y = map->grid_side / 2;
					tile_t* crystal_cell = map_crystal_cell(map);
					if (crystal_cell != NULL)
					{
						map_cell_coords(map, crystal_cell, &crystal_x, &crystal_y);
					}
					bool move_x = rand() % 2 == 0 && x != crystal_x;
					if (move_x)
					{
						dst_x += x < crystal_x ? 1 : -1;
					}
					else
					{
						dst_y += y < crystal_y ? 1 : -1;
					}
				}

				if (dst_x < 0)
				{
					dst_x = 0;
				}
				else if (dst_x >= map->grid_side)
				{
					dst_x = map->grid_side - 1;
				}
				if (dst_y < 0)
				{
					dst_y = 0;
				}
				else if (dst_y >= map->grid_side)
				{
					dst_y = map->grid_side - 1;
				}

				tile_t* dst_cell = map_tile(map, dst_x, dst_y);
				if (dst_cell->obj.type != OBJ_CRYSTAL &&
					dst_cell->obj.type != OBJ_UNIT_RED &&
					dst_cell->obj.type != OBJ_TOWER &&
					dst_cell->obj.type != OBJ_TOWER_OFF &&
					dst_cell->obj.type != OBJ_TREE &&
					(dst_cell == src_cell || dst_cell->obj.type != OBJ_NONE))
				{
					src_cell->obj.can_still_act = false;
					return false;
				}

				bool lay_egg = rand() % 11 == 0;

				map->motion.t = 0;
				map->motion.t_max = 6;
				map_cell_coords(map, src_cell, &map->motion.src_x, &map->motion.src_y);
				map_cell_coords(map, dst_cell, &map->motion.dst_x, &map->motion.dst_y);
				src_cell->obj.can_still_act = false;
				if (lay_egg)
				{
					map->motion.obj = (obj_t){.type = OBJ_ENEMY_EGG};
				}
				else
				{
					map->motion.obj = src_cell->obj;
					src_cell->obj = (obj_t){0};
				}

				return false;
			}
			else
			{
				assert(false);
			}
		}
	}

	if ((rand() >> 3) % 2 == 0 &&
		map_tile(map, 0, 0)->obj.type != OBJ_ENEMY_FLY)
	{
		map->motion.t = 0;
		map->motion.t_max = 6;
		map->motion.src_x = -1;
		map->motion.src_y = -1;
		map->motion.dst_x = 0;
		map->motion.dst_y = 0;
		map->motion.obj = (obj_t){.type = OBJ_ENEMY_FLY};
		map->motion.obj.can_still_act = false;
		return false;
	}

	if ((rand() >> 3) % 2 == 0 &&
		map_tile(map, map->grid_side-1, 0)->obj.type != OBJ_ENEMY_FLY)
	{
		map->motion.t = 0;
		map->motion.t_max = 6;
		map->motion.src_x = map->grid_side;
		map->motion.src_y = -1;
		map->motion.dst_x = map->grid_side-1;
		map->motion.dst_y = 0;
		map->motion.obj = (obj_t){.type = OBJ_ENEMY_FLY};
		map->motion.obj.can_still_act = false;
		return false;
	}

	if (gs->turn > 10 && gs->turn % 7 == 0 &&
		map_tile(map, 0, map->grid_side-1)->obj.type != OBJ_ENEMY_BIG)
	{
		map->motion.t = 0;
		map->motion.t_max = 6;
		map->motion.src_x = -1;
		map->motion.src_y = map->grid_side;
		map->motion.dst_x = 0;
		map->motion.dst_y = map->grid_side-1;
		map->motion.obj = (obj_t){.type = OBJ_ENEMY_BIG};
		map->motion.obj.can_still_act = false;
		return false;
	}

	if (gs->turn > 10 && gs->turn % 7 == 1 &&
		map_tile(map, map->grid_side-1, map->grid_side-1)->obj.type != OBJ_ENEMY_BIG)
	{
		map->motion.t = 0;
		map->motion.t_max = 6;
		map->motion.src_x = map->grid_side;
		map->motion.src_y = map->grid_side;
		map->motion.dst_x = map->grid_side-1;
		map->motion.dst_y = map->grid_side-1;
		map->motion.obj = (obj_t){.type = OBJ_ENEMY_BIG};
		map->motion.obj.can_still_act = false;
		return false;
	}

	return true;
}

void tower_shoot(map_t* map, int tower_x, int tower_y, int target_x, int target_y)
{
	map->motion.t = 0;
	map->motion.t_max = 12;
	map->motion.src_x = tower_x;
	map->motion.src_y = tower_y;
	map->motion.dst_x = target_x;
	map->motion.dst_y = target_y;
	map->motion.obj = (obj_t){.type = OBJ_SHOT};
	tile_t* tile = map_tile(map, tower_x, tower_y);
	tile->obj.can_still_act = false;
	tile->obj.ammo--;
	if (tile->obj.ammo <= 0)
	{
		tile->obj.type = OBJ_TOWER_OFF;
	}
}

bool game_play_towers(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		tile_t* tower_cell = map_tile(map, x, y);
		if (tower_cell->obj.can_still_act)
		{
			assert(tower_cell->obj.type == OBJ_TOWER);

			struct dir_t
			{
				int dx, dy;
				int x, y;
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
				dirs[i].x = x + dirs[i].dx;
				dirs[i].y = y + dirs[i].dy;
				dirs[i].is_valid =
					dirs[i].x >= 0 && dirs[i].x < map->grid_side &&
					dirs[i].y >= 0 && dirs[i].y < map->grid_side &&
					map_tile(map, dirs[i].x, dirs[i].y)->obj.type == OBJ_NONE;
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

					dirs[i].x += dirs[i].dx;
					dirs[i].y += dirs[i].dy;
					if (!(dirs[i].x >= 0 && dirs[i].x < map->grid_side &&
						dirs[i].y >= 0 && dirs[i].y < map->grid_side))
					{
						dirs[i].is_valid = false;
						continue;
					}
					obj_type_t type = map_tile(map, dirs[i].x, dirs[i].y)->obj.type;
					if (type == OBJ_ENEMY_FLY ||
						type == OBJ_ENEMY_BIG ||
						type == OBJ_ENEMY_EGG)
					{
						tower_shoot(map, x, y, dirs[i].x, dirs[i].y);
						return false;
					}
					if (type != OBJ_NONE)
					{
						dirs[i].is_valid = false;
					}
				}
			}

			tower_cell->obj.can_still_act = false;
			return false;
		}
	}

	return true;
}

void start_enemy_phase(game_state_t* gs, map_t* map)
{
	gs->player_phase = false;
	gs->tower_phase = false;
	gs->t = 0;
	map_clear_green(map);
	map_clear_selected(map);
	map_clear_can_still_act(map);

	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		tile_t* tile = map_tile(map, x, y);
		if (tile->obj.type == OBJ_ENEMY_FLY ||
			tile->obj.type == OBJ_ENEMY_BIG ||
			tile->obj.type == OBJ_ENEMY_EGG)
		{
			tile->obj.can_still_act = true;
		}
	}
}

void start_tower_phase(game_state_t* gs, map_t* map)
{
	gs->player_phase = false;
	gs->tower_phase = true;
	gs->t = 0;
	map_clear_green(map);
	map_clear_selected(map);
	map_clear_can_still_act(map);

	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		tile_t* tile = map_tile(map, x, y);
		if (tile->obj.type == OBJ_TOWER)
		{
			tile->obj.can_still_act = true;
		}
	}
}

void start_player_phase(game_state_t* gs, map_t* map)
{
	if (gs->game_is_lost)
	{
		start_enemy_phase(gs, map);
		return;
	}

	gs->turn++;
	gs->player_phase = true;
	gs->tower_phase = false;
	gs->tower_available = true;
	gs->t = 0;
	map_clear_can_still_act(map);

	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		tile_t* tile = map_tile(map, x, y);
		if (tile->obj.type == OBJ_UNIT_RED)
		{
			tile->obj.can_still_act = true;
		}
	}
}

void game_perform(game_state_t* gs, map_t* map)
{
	if (map->motion.t_max > 0)
	{
		if (map->motion.t >= map->motion.t_max)
		{
			tile_t* dst_cell = map_tile(map, map->motion.dst_x, map->motion.dst_y);
			if (dst_cell->obj.type == OBJ_CRYSTAL)
			{
				gs->game_is_lost = true;
				printf("Crystal destroyed, game over on turn %d\n", gs->turn);
			}
			if (map->motion.obj.type == OBJ_SHOT)
			{
				dst_cell->obj = (obj_t){0};
			}
			else
			{
				dst_cell->obj = map->motion.obj;
			}
			map->motion = (motion_t){0};
		}
		map->motion.t++;
	}
	else if (!gs->player_phase && !gs->tower_phase)
	{
		gs->t++;
		if (gs->t % 3 == 0)
		{
			bool const enemy_is_done = game_play_enemy(map, gs);
			if (enemy_is_done)
			{
				start_tower_phase(gs, map);
			}
		}
	}
	else if (gs->tower_phase)
	{
		gs->t++;
		if (gs->t % 3 == 0)
		{
			bool const towers_are_done = game_play_towers(map);
			if (towers_are_done)
			{
				start_player_phase(gs, map);
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

	g_font = TTF_OpenFontRW(SDL_RWFromConstMem(
		g_asset_font, g_asset_font_size), 0, 20);
	assert(g_font != NULL);

	init_sprite_sheet();
	
	map_t* map = malloc(sizeof(map_t));
	map->grid_side = WINDOW_SIDE / CELL_SIDE_PIXELS;
	map->grid = calloc(map->grid_side * map->grid_side, sizeof(tile_t));
	map->motion = (motion_t){0};
	map_generate(map);

	game_state_t* gs = malloc(sizeof(game_state_t));
	gs->tower_available = true;
	gs->player_phase = true;
	gs->tower_phase = false;
	gs->t = 0;
	gs->game_is_lost = false;
	gs->turn = 0;

	start_player_phase(gs, map);

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
							if (gs->player_phase)
							{
								start_enemy_phase(gs, map);
							}
						break;
					}
				break;
				case SDL_MOUSEMOTION:
					handle_mouse_motion(map, event.motion.x, event.motion.y);
				break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_MIDDLE && gs->player_phase)
					{
						start_enemy_phase(gs, map);
					}
					if (gs->player_phase)
					{
						handle_mouse_click(map, gs,
							event.button.button == SDL_BUTTON_LEFT);
					}
				break;
			}
		}

		game_perform(gs, map);

		SDL_SetRenderDrawColor(g_renderer, 30, 40, 80, 255);
		SDL_RenderClear(g_renderer);
		
		draw_map(map);

		char string[60];
		sprintf(string, "TURN %d", gs->turn);
		draw_text(string, 0, 0, 0, 0, 0, false);
		if (gs->game_is_lost)
		{
			draw_text("GAME OVER", 0, 0, 0, 0, 20, false);
		}
		
		SDL_RenderPresent(g_renderer);
	}

	if (!gs->game_is_lost)
	{
		printf("Quit on turn %d\n", gs->turn);
	}

	TTF_Quit();
	IMG_Quit(); 
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit(); 
	return 0;
}
