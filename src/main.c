
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "embedded.h"

enum floor_type_t
{
	FLOOR_FIELD,
	FLOOR_DESERT,
	FLOOR_WATER,
};
typedef enum floor_type_t floor_type_t;

struct cell_t
{
	floor_type_t floor;
	bool has_mountain;
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

	if (cell->has_mountain)
	{
		SDL_SetRenderDrawColor(renderer, 80, 80, 0, 255);
		rect.x += 4;
		rect.y -= 2;
		rect.w -= 8;
		rect.h -= 2;
		SDL_RenderFillRect(renderer, &rect);
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

	#define CELL_SIDE_PIXELS 40
	#define GRID_SIDE (800 / CELL_SIDE_PIXELS)
	cell_t grid[GRID_SIDE * GRID_SIDE] = {0};

	for (int y = 0; y < GRID_SIDE; y++)
	for (int x = 0; x < GRID_SIDE; x++)
	{
		if ((x ^ (y + 1)) % 7 == 1)
		{
			grid[y * GRID_SIDE + x].floor = FLOOR_DESERT;
		}

		if ((x ^ (y + 3)) % 13 == 3)
		{
			grid[y * GRID_SIDE + x].has_mountain = true;
		}
	}

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
			}
		}

		SDL_SetRenderDrawColor(renderer, 30, 40, 80, 255);
		SDL_RenderClear(renderer);

		#if 0
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		for (int x = 0; x < 800; x += 40)
		{
			SDL_RenderDrawLine(renderer, x, 0, x, 800);
		}
		for (int y = 0; y < 800; y += 40)
		{
			SDL_RenderDrawLine(renderer, 0, y, 800, y);
		}
		#endif

		for (int y = 0; y < GRID_SIDE; y++)
		for (int x = 0; x < GRID_SIDE; x++)
		{
			cell_t const* cell = &grid[y * GRID_SIDE + x];
			int const pixel_x = x * CELL_SIDE_PIXELS;
			int const pixel_y = y * CELL_SIDE_PIXELS;
			draw_cell(renderer, cell, pixel_x, pixel_y, CELL_SIDE_PIXELS);
		}

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyWindow(window);
	SDL_Quit(); 

	return 0;
}
