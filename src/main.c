
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "embedded.h"

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

void draw_object(SDL_Renderer* renderer, object_t const* object, int x, int y, int side,
	bool is_hovered, bool is_selected)
{
	if (*object == OBJECT_NONE)
	{
		return;
	}

	SDL_Rect rect = {.x = x, .y = y, .w = side, .h = side};
	switch (*object)
	{
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
	if (is_selected)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}
	else if (is_hovered)
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}
}

struct cell_t
{
	floor_type_t floor;
	object_t object;
	bool is_green;
};
typedef struct cell_t cell_t;

void draw_cell(SDL_Renderer* renderer, cell_t const* cell, int x, int y, int side,
	bool is_hovered, bool is_selected)
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
	if (is_selected)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}
	else if (is_hovered)
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}
	else if (cell->is_green)
	{
		SDL_SetRenderDrawColor(renderer, 100, 255, 0, 255);
		SDL_RenderDrawRect(renderer, &rect);
	}

	draw_object(renderer, &cell->object, x, y, side, is_hovered, is_selected);
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
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 800, 0);
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

	#define CELL_SIDE_PIXELS 40

	SDL_Surface* sprite_sheet_surface = IMG_LoadPNG_RW(SDL_RWFromConstMem(
		g_asset_sprite_sheet_png, g_asset_sprite_sheet_png_size));
	SDL_Texture* sprite_sheet_texture = SDL_CreateTextureFromSurface(renderer,
		sprite_sheet_surface);
	SDL_FreeSurface(sprite_sheet_surface);
	SDL_Rect sprite_sheet_rect = {.w = 16, .h = 16};
	SDL_Rect sprite_rect = {.w = CELL_SIDE_PIXELS, .h = CELL_SIDE_PIXELS};
	
	#define GRID_SIDE (800 / CELL_SIDE_PIXELS)
	cell_t grid[GRID_SIDE * GRID_SIDE] = {0};

	for (int y = 0; y < GRID_SIDE; y++)
	for (int x = 0; x < GRID_SIDE; x++)
	{
		if ((x ^ (y + 1)) % 7 == 1)
		{
			grid[y * GRID_SIDE + x].floor = FLOOR_DESERT;
		}

		if (((x - 2) ^ (y + 2)) % 31 == 1)
		{
			grid[y * GRID_SIDE + x].object = OBJECT_UNIT_ENEMY;
		}
		if ((x ^ (y + 3)) % 13 == 3)
		{
			grid[y * GRID_SIDE + x].object = OBJECT_ROCK;
		}

	}

	grid[(GRID_SIDE / 2) * GRID_SIDE + (GRID_SIDE / 2)].object = OBJECT_UNIT_CONTROLLED;
	grid[(GRID_SIDE / 2) * GRID_SIDE + (GRID_SIDE / 2 - 1)].object = OBJECT_UNIT_CONTROLLED;
	grid[(GRID_SIDE / 2) * GRID_SIDE + (GRID_SIDE / 2 - 2)].object = OBJECT_UNIT_CONTROLLED;

	int mouse_pixel_x = -1, mouse_pixel_y = -1;
	int hovered_cell_x = -1, hovered_cell_y = -1;
	int selected_cell_x = -1, selected_cell_y = -1;

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
					mouse_pixel_x = event.motion.x;
					mouse_pixel_y = event.motion.y;
					hovered_cell_x = mouse_pixel_x / CELL_SIDE_PIXELS;
					hovered_cell_y = mouse_pixel_y / CELL_SIDE_PIXELS;
				break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						int const old_selected_cell_x = selected_cell_x;
						int const old_selected_cell_y = selected_cell_y;
						selected_cell_x = hovered_cell_x;
						selected_cell_y = hovered_cell_y;
						cell_t* selected_cell =
							&grid[selected_cell_y * GRID_SIDE + selected_cell_x];
						if (selected_cell->is_green)
						{
							cell_t* old_selected_cell =
								&grid[old_selected_cell_y * GRID_SIDE + old_selected_cell_x];
							assert(old_selected_cell->object == OBJECT_UNIT_CONTROLLED);
							old_selected_cell->object = OBJECT_NONE;
							selected_cell->object = OBJECT_UNIT_CONTROLLED;
							for (int y = 0; y < GRID_SIDE; y++)
							for (int x = 0; x < GRID_SIDE; x++)
							{
								grid[y * GRID_SIDE + x].is_green = false;
							}
							break;
						}
						for (int y = 0; y < GRID_SIDE; y++)
						for (int x = 0; x < GRID_SIDE; x++)
						{
							grid[y * GRID_SIDE + x].is_green = false;
						}
						if (selected_cell->object == OBJECT_UNIT_CONTROLLED)
						{
							for (int y = 0; y < GRID_SIDE; y++)
							for (int x = 0; x < GRID_SIDE; x++)
							{
								int const dist_x = abs(x - selected_cell_x);
								int const dist_y = abs(y - selected_cell_y);
								int const dist = dist_x + dist_y;
								bool is_accessible = dist != 0 && dist <= 2;
								if (grid[y * GRID_SIDE + x].object != OBJECT_NONE)
								{
									is_accessible = false;
								}
								grid[y * GRID_SIDE + x].is_green = is_accessible;
							}
						}
					}
				break;
			}
		}

		SDL_SetRenderDrawColor(renderer, 30, 40, 80, 255);
		SDL_RenderClear(renderer);

		for (int y = 0; y < GRID_SIDE; y++)
		for (int x = 0; x < GRID_SIDE; x++)
		{
			cell_t const* cell = &grid[y * GRID_SIDE + x];
			int const pixel_x = x * CELL_SIDE_PIXELS;
			int const pixel_y = y * CELL_SIDE_PIXELS;
			bool const is_hovered = x == hovered_cell_x && y == hovered_cell_y;
			bool const is_selected = x == selected_cell_x && y == selected_cell_y;
			draw_cell(renderer, cell, pixel_x, pixel_y, CELL_SIDE_PIXELS,
				is_hovered, is_selected);
		}

		sprite_sheet_rect.x = 0;
		sprite_sheet_rect.y = 0;
		sprite_rect.x = CELL_SIDE_PIXELS * ((800 / CELL_SIDE_PIXELS) / 2);
		sprite_rect.y = CELL_SIDE_PIXELS * ((800 / CELL_SIDE_PIXELS) / 2 - 1);
		SDL_RenderCopy(renderer, sprite_sheet_texture, &sprite_sheet_rect, &sprite_rect);

		SDL_RenderPresent(renderer);
	}

	IMG_Quit(); 
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit(); 

	return 0;
}
