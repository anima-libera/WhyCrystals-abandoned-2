
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "embedded.h"

#define CELL_SIDE_PIXELS 64
#define WINDOW_SIDE (800 - 800 % CELL_SIDE_PIXELS)

enum floor_type_t
{
	FLOOR_FIELD,
	FLOOR_DESERT,
	FLOOR_WATER,
};
typedef enum floor_type_t floor_type_t;

enum object_t
{
	OBJECT_NONE,
	OBJECT_ROCK,
	OBJECT_UNIT_CONTROLLED,
	OBJECT_UNIT_ENEMY,
};
typedef enum object_t object_t;

void draw_object(SDL_Renderer* renderer, object_t const* object, int x, int y, int side)
{
	SDL_Rect rect = {.x = x, .y = y, .w = side, .h = side};
	switch (*object)
	{
		case OBJECT_NONE:
			return;
		break;
		case OBJECT_ROCK:
			SDL_SetRenderDrawColor(renderer, 80, 80, 0, 255);
			rect.x += 4;
			rect.y -= 2;
			rect.w -= 8;
			rect.h -= 2;
		break;
		case OBJECT_UNIT_CONTROLLED:
			SDL_SetRenderDrawColor(renderer, 0, 0, 160, 255);
			rect.x += 10;
			rect.y += 5;
			rect.w -= 20;
			rect.h -= 10;
		break;
		case OBJECT_UNIT_ENEMY:
			SDL_SetRenderDrawColor(renderer, 160, 0, 0, 255);
			rect.x += 10;
			rect.y += 5;
			rect.w -= 20;
			rect.h -= 10;
		break;
	}
	SDL_RenderFillRect(renderer, &rect);
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

void draw_cell(SDL_Renderer* renderer, cell_t const* cell, int x, int y, int side)
{
	SDL_Rect rect = {.x = x, .y = y, .w = side, .h = side};
	switch (cell->floor)
	{
		case FLOOR_FIELD:
			SDL_SetRenderDrawColor(renderer, 40, 210, 10, 255);
		break;
		case FLOOR_DESERT:
			SDL_SetRenderDrawColor(renderer, 250, 210, 10, 255);
		break;
		case FLOOR_WATER:
			SDL_SetRenderDrawColor(renderer, 40, 210, 250, 255);
		break;
	}
	SDL_RenderFillRect(renderer, &rect);
	if (cell->is_selected)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}
	else if (cell->is_hovered && !cell->is_green)
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}
	if (cell->is_green)
	{
		int const margin = cell->is_hovered ? 3 : 5;
		rect.x += margin;
		rect.y += margin;
		rect.w -= margin * 2;
		rect.h -= margin * 2;
		SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}

	draw_object(renderer, &cell->object, x, y, side);
}

struct map_t
{
	int grid_side;
	cell_t* grid;
};
typedef struct map_t map_t;

cell_t* map_cell(map_t* map, int x, int y)
{
	return &map->grid[y * map->grid_side + x];
}

void map_generate(map_t* map)
{
	for (int y = 0; y < map->grid_side; y++)
	for (int x = 0; x < map->grid_side; x++)
	{
		if ((x ^ (y + 1)) % 7 == 1)
		{
			map_cell(map, x, y)->floor = FLOOR_DESERT;
		}

		if (((x - 2) ^ (y + 2)) % 31 == 1)
		{
			map_cell(map, x, y)->object = OBJECT_UNIT_ENEMY;
		}
		if ((x ^ (y + 3)) % 13 == 3)
		{
			map_cell(map, x, y)->object = OBJECT_ROCK;
		}

	}

	map_cell(map, 6, 6)->object = OBJECT_UNIT_CONTROLLED;
	map_cell(map, 7, 6)->object = OBJECT_UNIT_CONTROLLED;
	map_cell(map, 8, 6)->object = OBJECT_UNIT_CONTROLLED;
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

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (renderer == NULL)
	{
		printf("TODO: Handle error.\n");
		exit(-1);
	}

	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG)
	{
		printf("TODO: Handle error.\n");
		exit(-1);
	}

	SDL_Surface* sprite_sheet_surface = IMG_LoadPNG_RW(SDL_RWFromConstMem(
		g_asset_sprite_sheet_png, g_asset_sprite_sheet_png_size));
	SDL_Texture* sprite_sheet_texture = SDL_CreateTextureFromSurface(renderer,
		sprite_sheet_surface);
	SDL_FreeSurface(sprite_sheet_surface);
	SDL_Rect sprite_sheet_rect = {.w = 16, .h = 16};
	SDL_Rect sprite_rect = {.w = CELL_SIDE_PIXELS, .h = CELL_SIDE_PIXELS};
	
	map_t* map = malloc(sizeof(map_t));
	map->grid_side = WINDOW_SIDE / CELL_SIDE_PIXELS;
	map->grid = calloc(map->grid_side * map->grid_side, sizeof(cell_t));
	map_generate(map);

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
					}
				break;
				case SDL_MOUSEMOTION:
				{
					map_clear_hovered(map);
					cell_t* cell = map_cell_from_pixel(map, event.motion.x, event.motion.y);
					cell->is_hovered = true;
				}
				break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						cell_t* old_selected = map_selected_cell(map);
						map_clear_selected(map);
						cell_t* new_selected = map_hovered_cell(map);
						new_selected->is_selected = true;
						if (new_selected->is_green)
						{
							assert(old_selected->object == OBJECT_UNIT_CONTROLLED);
							old_selected->object = OBJECT_NONE;
							new_selected->object = OBJECT_UNIT_CONTROLLED;
							map_clear_green(map);
							break;
						}
						map_clear_green(map);
						if (new_selected->object == OBJECT_UNIT_CONTROLLED)
						{
							int selected_x, selected_y;
							map_cell_coords(map, new_selected, &selected_x, &selected_y);
							for (int y = 0; y < map->grid_side; y++)
							for (int x = 0; x < map->grid_side; x++)
							{
								int const dist = abs(x - selected_x) + abs(y - selected_y);
								bool is_accessible = dist != 0 && dist <= 2;
								if (map_cell(map, x, y)->object != OBJECT_NONE)
								{
									is_accessible = false;
								}
								map_cell(map, x, y)->is_green = is_accessible;
							}
						}
					}
				break;
			}
		}

		SDL_SetRenderDrawColor(renderer, 30, 40, 80, 255);
		SDL_RenderClear(renderer);

		for (int y = 0; y < map->grid_side; y++)
		for (int x = 0; x < map->grid_side; x++)
		{
			int const pixel_x = x * CELL_SIDE_PIXELS;
			int const pixel_y = y * CELL_SIDE_PIXELS;
			draw_cell(renderer, map_cell(map, x, y),
				pixel_x, pixel_y, CELL_SIDE_PIXELS);
		}

		sprite_sheet_rect.x = 0;
		sprite_sheet_rect.y = 0;
		sprite_rect.x = CELL_SIDE_PIXELS * ((WINDOW_SIDE / CELL_SIDE_PIXELS) / 2);
		sprite_rect.y = CELL_SIDE_PIXELS * ((WINDOW_SIDE / CELL_SIDE_PIXELS) / 2 - 1);
		SDL_RenderCopy(renderer, sprite_sheet_texture, &sprite_sheet_rect, &sprite_rect);

		SDL_RenderPresent(renderer);
	}

	IMG_Quit(); 
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit(); 

	return 0;
}
