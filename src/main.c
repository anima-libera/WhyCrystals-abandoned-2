
#include <stdio.h>
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

	SDL_Delay(1000);

	SDL_DestroyWindow(window);
	SDL_Quit(); 

	return 0;
}
