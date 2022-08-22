
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "embedded.h"

SDL_Renderer* g_renderer;

#define CELL_SIDE_PIXELS 64
#define WINDOW_SIDE (800 - 800 % CELL_SIDE_PIXELS)

enum floor_type_t
{
	FLOOR_FIELD,
	FLOOR_DESERT,
	FLOOR_WATER,
};
typedef enum floor_type_t floor_type_t;

struct sprite_sheet_t
{
	SDL_Texture* texture;
	SDL_Rect rect_rock;
	SDL_Rect rect_unit_controlled;
	SDL_Rect rect_unit_enemy;
	SDL_Rect rect_egg;
	SDL_Rect rect_tower;
	SDL_Rect rect_shot;
	SDL_Rect rect_tree;
	SDL_Rect rect_crystal;
	SDL_Rect rect_grassland;
	SDL_Rect rect_desert;
};
typedef struct sprite_sheet_t sprite_sheet_t;

sprite_sheet_t g_ss;

void sprite_sheet_load(sprite_sheet_t* ss)
{
	SDL_Surface* surface = IMG_LoadPNG_RW(SDL_RWFromConstMem(
		g_asset_sprite_sheet_png, g_asset_sprite_sheet_png_size));
	ss->texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	SDL_FreeSurface(surface);
	SDL_Rect rect = {.x = 0, .y = 0, .w = 16, .h = 16};
	rect.y +=  0; ss->rect_rock = rect;
	rect.y += 16; ss->rect_unit_controlled = rect;
	rect.y += 16; ss->rect_unit_enemy = rect;
	rect.y += 16; ss->rect_egg = rect;
	rect.y += 16; ss->rect_tower = rect;
	rect.y += 16; ss->rect_shot = rect;
	rect.y += 16; ss->rect_tree = rect;
	rect.y += 16; ss->rect_crystal = rect;
	rect.y += 16; ss->rect_grassland = rect;
	rect.y += 16; ss->rect_desert = rect;
}

enum object_type_t
{
	OBJECT_NONE,
	OBJECT_ROCK,
	OBJECT_UNIT_CONTROLLED,
	OBJECT_UNIT_ENEMY,
	OBJECT_TOWER,
	OBJECT_SHOT,
	OBJECT_CRYSTAL,
	OBJECT_EGG,
};
typedef enum object_type_t object_type_t;

struct object_t
{
	object_type_t type;
	bool can_still_move;
};
typedef struct object_t object_t;

void draw_object(object_t const* object, int x, int y, int side)
{
	SDL_Rect dst_rect = {.x = x, .y = y, .w = side, .h = side};
	SDL_Rect* src_rect;
	switch (object->type)
	{
		case OBJECT_NONE:
			return;
		break;
		case OBJECT_ROCK:
			src_rect = &g_ss.rect_rock;
		break;
		case OBJECT_UNIT_CONTROLLED:
			src_rect = &g_ss.rect_unit_controlled;
		break;
		case OBJECT_UNIT_ENEMY:
			src_rect = &g_ss.rect_unit_enemy;
		break;
		case OBJECT_TOWER:
			src_rect = &g_ss.rect_tower;
		break;
		case OBJECT_SHOT:
			src_rect = &g_ss.rect_shot;
		break;
		case OBJECT_CRYSTAL:
			src_rect = &g_ss.rect_crystal;
		break;
		case OBJECT_EGG:
			src_rect = &g_ss.rect_egg;
		break;
	}
	SDL_RenderCopy(g_renderer, g_ss.texture, src_rect, &dst_rect);

	if (object->can_still_move)
	{
		dst_rect.x += 4;
		dst_rect.y += 4;
		dst_rect.w = 8;
		dst_rect.h = 8;
		SDL_SetRenderDrawColor(g_renderer, 0, 100, 0, 255);
		SDL_RenderDrawRect(g_renderer, &dst_rect);
	}
}

struct cell_t
{
	floor_type_t floor;
	object_t object;
	bool is_green;
	bool is_hovered;
	bool is_selected;
};
typedef struct cell_t cell_t;

void draw_cell(cell_t const* cell, int x, int y, int side)
{
	SDL_Rect dst_rect = {.x = x, .y = y, .w = side, .h = side};
	SDL_Rect* src_rect;
	switch (cell->floor)
	{
		case FLOOR_FIELD:
			src_rect = &g_ss.rect_grassland;
		break;
		case FLOOR_DESERT:
			src_rect = &g_ss.rect_desert;
		break;
		case FLOOR_WATER:
			printf("TODO: water tile\n");
			exit(-1);
		break;
	}
	SDL_RenderCopy(g_renderer, g_ss.texture, src_rect, &dst_rect);
	switch (cell->floor)
	{
		case FLOOR_FIELD:
			SDL_SetRenderDrawColor(g_renderer, 0, 255, 0, 180);
		break;
		case FLOOR_DESERT:
			SDL_SetRenderDrawColor(g_renderer, 255, 255, 0, 110);
		break;
		case FLOOR_WATER:
			printf("TODO: water tile\n");
			exit(-1);
		break;
	}
	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(g_renderer, &dst_rect);
	SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

	if (cell->is_selected)
	{
		SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(g_renderer, &dst_rect);
	}
	else if (cell->is_hovered && !cell->is_green)
	{
		SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(g_renderer, &dst_rect);
	}
	if (cell->is_green)
	{
		int const margin = cell->is_hovered ? 3 : 5;
		dst_rect.x += margin;
		dst_rect.y += margin;
		dst_rect.w -= margin * 2;
		dst_rect.h -= margin * 2;
		SDL_SetRenderDrawColor(g_renderer, 0, 100, 0, 255);
		SDL_RenderDrawRect(g_renderer, &dst_rect);
	}

	draw_object(&cell->object, x, y, side);
}

struct motion_t
{
	int t;
	int t_max;
	int src_x, src_y;
	int dst_x, dst_y;
	object_t object;
};
typedef struct motion_t motion_t;

struct map_t
{
	int grid_side;
	cell_t* grid;
	motion_t motion;
};
typedef struct map_t map_t;

cell_t* map_cell(map_t* map, int x, int y)
{
	return &map->grid[y * map->grid_side + x];
}

void draw_map(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		draw_cell(map_cell(map, x, y),
			x * CELL_SIDE_PIXELS, y * CELL_SIDE_PIXELS, CELL_SIDE_PIXELS);
	}

	if (map->motion.t_max > 0)
	{
		motion_t* m = &map->motion;
		float r = (float)m->t / (float)m->t_max;
		float x = (float)m->src_x * (1.0f - r) + (float)m->dst_x * r;
		float y = (float)m->src_y * (1.0f - r) + (float)m->dst_y * r;
		draw_object(&m->object,
			x * (float)CELL_SIDE_PIXELS, y * (float)CELL_SIDE_PIXELS,
			CELL_SIDE_PIXELS);
	}
}

void map_generate(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_cell(map, x, y)->object = (object_t){0};

		if (rand() % 7 == 1)
		{
			map_cell(map, x, y)->floor = FLOOR_DESERT;
		}

		if (rand() % 21 == 1)
		{
			map_cell(map, x, y)->object.type = OBJECT_UNIT_ENEMY;
		}
		if (rand() % 13 == 3)
		{
			map_cell(map, x, y)->object.type = OBJECT_ROCK;
		}
	}

	map_cell(map, 4, 5)->object.type = OBJECT_CRYSTAL;

	map_cell(map, 5, 6)->object.type = OBJECT_UNIT_CONTROLLED;
	map_cell(map, 5, 6)->object.can_still_move = true;
	map_cell(map, 6, 6)->object.type = OBJECT_UNIT_CONTROLLED;
	map_cell(map, 6, 6)->object.can_still_move = true;
	map_cell(map, 7, 6)->object.type = OBJECT_UNIT_CONTROLLED;
	map_cell(map, 7, 6)->object.can_still_move = true;

	map_cell(map, 2, 7)->object.type = OBJECT_TOWER;
}

void map_clear_green(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_cell(map, x, y)->is_green = false;
	}
}

void map_clear_hovered(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_cell(map, x, y)->is_hovered = false;
	}
}

void map_clear_selected(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_cell(map, x, y)->is_selected = false;
	}
}

void map_clear_can_still_move(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		map_cell(map, x, y)->object.can_still_move = false;
	}
}

cell_t* map_cell_from_pixel(map_t* map, int pixel_x, int pixel_y)
{
	return map_cell(map,
		pixel_x / CELL_SIDE_PIXELS, pixel_y / CELL_SIDE_PIXELS);
}

cell_t* map_hovered_cell(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		cell_t* cell = map_cell(map, x, y);
		if (cell->is_hovered)
		{
			return cell;
		}
	}
	return NULL;
}

cell_t* map_selected_cell(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		cell_t* cell = map_cell(map, x, y);
		if (cell->is_selected)
		{
			return cell;
		}
	}
	return NULL;
}

void map_cell_coords(map_t* map, cell_t* cell, int* out_x, int* out_y)
{
	int cell_offset = cell - map->grid;
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
	cell_t* cell = map_cell_from_pixel(map, new_x, new_y);
	cell->is_hovered = true;
}

struct game_state_t
{
	bool tower_available;
	bool player_phase;
	bool tower_phase;
	int t;
	bool game_is_lost;
	int turn_number;
};
typedef struct game_state_t game_state_t;

void handle_mouse_click(map_t* map, game_state_t* gs, bool is_left_click)
{
	cell_t* old_selected = map_selected_cell(map);
	map_clear_selected(map);
	cell_t* new_selected = map_hovered_cell(map);
	new_selected->is_selected = true;
	if (map->motion.t_max > 0)
	{
		return;
	}
	if (new_selected->is_green)
	{
		assert(old_selected->object.type == OBJECT_UNIT_CONTROLLED);
		map->motion.t = 0;
		map->motion.t_max = 7;
		map_cell_coords(map, old_selected, &map->motion.src_x, &map->motion.src_y);
		map_cell_coords(map, new_selected, &map->motion.dst_x, &map->motion.dst_y);
		if (is_left_click)
		{
			map->motion.object = old_selected->object;
			map->motion.object.can_still_move = false;
			old_selected->object = (object_t){0};
		}
		else if (gs->tower_available)
		{
			map->motion.object = (object_t){.type = OBJECT_TOWER};
			gs->tower_available = false;
			old_selected->object.can_still_move = false;
		}
		map_clear_green(map);
		return;
	}
	map_clear_green(map);
	if (new_selected->object.type == OBJECT_UNIT_CONTROLLED &&
		new_selected->object.can_still_move)
	{
		int selected_x, selected_y;
		map_cell_coords(map, new_selected, &selected_x, &selected_y);
		for (int y = 0; y < map->grid_side; y++)
		for (int x = 0; x < map->grid_side; x++)
		{
			int const dist = abs(x - selected_x) + abs(y - selected_y);
			bool is_accessible = dist != 0 && dist <= 2;
			if (map_cell(map, x, y)->object.type != OBJECT_NONE)
			{
				is_accessible = false;
			}
			map_cell(map, x, y)->is_green = is_accessible;
		}
	}
}

bool game_play_enemy(map_t* map, game_state_t* gs)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		cell_t* src_cell = map_cell(map, x, y);
		if (src_cell->object.can_still_move)
		{
			if (src_cell->object.type == OBJECT_EGG)
			{
				src_cell->object.can_still_move = false;
				if (rand() % 4 == 0)
				{
					src_cell->object.type = OBJECT_UNIT_ENEMY;
				}
				return false;
			}

			assert(src_cell->object.type == OBJECT_UNIT_ENEMY);
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
				int const crystal_x = 4, crystal_y = 5;
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

			cell_t* dst_cell = map_cell(map, dst_x, dst_y);
			if (dst_cell->object.type != OBJECT_CRYSTAL &&
				dst_cell->object.type != OBJECT_UNIT_CONTROLLED &&
				dst_cell->object.type != OBJECT_TOWER &&
				(dst_cell == src_cell || dst_cell->object.type != OBJECT_NONE))
			{
				src_cell->object.can_still_move = false;
				return false;
			}

			bool lay_egg = rand() % 11 == 0;

			map->motion.t = 0;
			map->motion.t_max = 6;
			map_cell_coords(map, src_cell, &map->motion.src_x, &map->motion.src_y);
			map_cell_coords(map, dst_cell, &map->motion.dst_x, &map->motion.dst_y);
			src_cell->object.can_still_move = false;
			if (lay_egg)
			{
				map->motion.object = (object_t){.type = OBJECT_EGG};
			}
			else
			{
				map->motion.object = src_cell->object;
				src_cell->object = (object_t){0};
			}
			return false;
		}
	}

	if ((rand() >> 3) % 2 == 0 && map_cell(map, 0, 0)->object.type == OBJECT_NONE)
	{
		map->motion.t = 0;
		map->motion.t_max = 6;
		map->motion.src_x = -1;
		map->motion.src_y = -1;
		map->motion.dst_x = 0;
		map->motion.dst_y = 0;
		map->motion.object = (object_t){.type = OBJECT_UNIT_ENEMY};
		map->motion.object.can_still_move = false;
		return false;
	}

	if ((rand() >> 3) % 2 == 0 && map_cell(map, map->grid_side-1, 0)->object.type == OBJECT_NONE)
	{
		map->motion.t = 0;
		map->motion.t_max = 6;
		map->motion.src_x = map->grid_side;
		map->motion.src_y = -1;
		map->motion.dst_x = map->grid_side-1;
		map->motion.dst_y = 0;
		map->motion.object = (object_t){.type = OBJECT_UNIT_ENEMY};
		map->motion.object.can_still_move = false;
		return false;
	}

	if (gs->turn_number > 0 && gs->turn_number % 13 == 0)
	{
		for (int y = map->grid_side - 2; y < map->grid_side; y++)
		for (int x = 0; x < map->grid_side; x++)
		{
			if ((rand() >> 4) % 4 <= 2)
			{
				map_cell(map, x, y)->object.type = OBJECT_EGG;
			}
		}
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
	map->motion.object = (object_t){.type = OBJECT_SHOT};
	map_cell(map, tower_x, tower_y)->object.can_still_move = false;
}

bool game_play_towers(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		cell_t* tower_cell = map_cell(map, x, y);
		if (tower_cell->object.can_still_move)
		{
			assert(tower_cell->object.type == OBJECT_TOWER);

			struct dir_t
			{
				int dx, dy;
				int x, y;
				bool is_valid;
			};
			typedef struct dir_t dir_t;

			dir_t dirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
			for (int i = 0; i < 4; i ++)
			{
				dirs[i].x = x + dirs[i].dx;
				dirs[i].y = y + dirs[i].dy;
				dirs[i].is_valid =
					dirs[i].x >= 0 && dirs[i].x < map->grid_side &&
					dirs[i].y >= 0 && dirs[i].y < map->grid_side &&
					map_cell(map, dirs[i].x, dirs[i].y)->object.type == OBJECT_NONE;
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
					object_type_t type = map_cell(map, dirs[i].x, dirs[i].y)->object.type;
					if (type == OBJECT_UNIT_ENEMY || type == OBJECT_EGG)
					{
						tower_shoot(map, x, y, dirs[i].x, dirs[i].y);
						return false;
					}
					if (type != OBJECT_NONE)
					{
						dirs[i].is_valid = false;
					}
				}
			}

			tower_cell->object.can_still_move = false;
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
	map_clear_can_still_move(map);

	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		cell_t* cell = map_cell(map, x, y);
		if (cell->object.type == OBJECT_UNIT_ENEMY ||
			cell->object.type == OBJECT_EGG)
		{
			cell->object.can_still_move = true;
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
	map_clear_can_still_move(map);

	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		cell_t* cell = map_cell(map, x, y);
		if (cell->object.type == OBJECT_TOWER)
		{
			cell->object.can_still_move = true;
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

	gs->turn_number++;
	gs->player_phase = true;
	gs->tower_phase = false;
	gs->tower_available = true;
	gs->t = 0;
	map_clear_can_still_move(map);

	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		cell_t* cell = map_cell(map, x, y);
		if (cell->object.type == OBJECT_UNIT_CONTROLLED)
		{
			cell->object.can_still_move = true;
		}
	}
}

void game_perform(game_state_t* gs, map_t* map)
{
	if (map->motion.t_max > 0)
	{
		if (map->motion.t >= map->motion.t_max)
		{
			cell_t* dst_cell = map_cell(map, map->motion.dst_x, map->motion.dst_y);
			if (dst_cell->object.type == OBJECT_CRYSTAL)
			{
				gs->game_is_lost = true;
				printf("Crystal destroyed, game over on turn %d\n", gs->turn_number);
			}
			if (map->motion.object.type == OBJECT_SHOT)
			{
				dst_cell->object = (object_t){0};
			}
			else
			{
				dst_cell->object = map->motion.object;
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
	printf("hellow, %s", g_asset_test);

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("TODO: Handle error.\n");
		exit(-1);
	}

	SDL_Window* window = SDL_CreateWindow("Pyea",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_SIDE, WINDOW_SIDE, 0);
	if (window == NULL)
	{
		printf("TODO: Handle error.\n");
		exit(-1);
	}

	g_renderer = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (g_renderer == NULL)
	{
		printf("TODO: Handle error.\n");
		exit(-1);
	}

	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG)
	{
		printf("TODO: Handle error.\n");
		exit(-1);
	}

	sprite_sheet_load(&g_ss);
	
	map_t* map = malloc(sizeof(map_t));
	map->grid_side = WINDOW_SIDE / CELL_SIDE_PIXELS;
	map->grid = calloc(map->grid_side * map->grid_side, sizeof(cell_t));
	map->motion = (motion_t){0};
	map_generate(map);

	game_state_t* gs = malloc(sizeof(game_state_t));
	gs->tower_available = true;
	gs->player_phase = true;
	gs->tower_phase = false;
	gs->t = 0;
	gs->game_is_lost = false;
	gs->turn_number = 0;

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
		SDL_RenderPresent(g_renderer);
	}

	if (!gs->game_is_lost)
	{
		printf("Quit on turn %d\n", gs->turn_number);
	}

	IMG_Quit(); 
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(window);
	SDL_Quit(); 

	return 0;
}
