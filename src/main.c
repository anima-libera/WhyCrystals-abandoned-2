
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "embedded.h"

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

	int t = 0;

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

		t++;

		SDL_SetRenderDrawColor(renderer, 30, 40, 80, 255);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		for (int x = 0; x < 800; x += 40)
		{
			SDL_RenderDrawLine(renderer, x, 0, x, 800);
		}
		for (int y = 0; y < 800; y += 40)
		{
			SDL_RenderDrawLine(renderer, 0, y, 800, y);
		}

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyWindow(window);
	SDL_Quit(); 

	return 0;
}
