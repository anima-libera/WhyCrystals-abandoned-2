
#include "embedded.h"
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

int g_window_w = 1200, g_window_h = 600;
SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;
TTF_Font* g_font = NULL;

struct rgb_t
{
	uint8_t r, g, b;
};
typedef struct rgb_t rgb_t;

rgb_t cool_bg = {10, 40, 35};
rgb_t cool_h = {20, 80, 70};
rgb_t cool_white = {180, 220, 200};
rgb_t cool_yellow = {230, 240, 40};

void draw_text(char const* text, rgb_t color, SDL_Rect rect)
{
	/* Make a texture out of the text. */
	SDL_Color sdl_color = {color.r, color.g, color.b, 255};
	SDL_Surface* surface = TTF_RenderText_Solid(g_font, text, sdl_color);
	assert(surface != NULL);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(texture != NULL);
	SDL_FreeSurface(surface);

	//SDL_Rect texture_rect;
	//SDL_QueryTexture(texture, NULL, NULL, &texture_rect.w, &texture_rect.h);

	/* It looks better thay way. */
	rect.y -= 5;
	rect.h += 10;

	SDL_RenderCopy(g_renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
}

void draw_char(char c, rgb_t color, SDL_Rect rect)
{
	char text[] = {c, '\0'};
	draw_text(text, color, rect);
}

struct tile_t
{
	char c;
};
typedef struct tile_t tile_t;

int tile_w = 30, tile_h = 30;

/* Map grid. */
tile_t* mg = NULL;
int mg_w = -1, mg_h = -1;

struct tc_t
{
	int x, y;
};
typedef struct tc_t tc_t;

tc_t player_tc;

int main(void)
{
	srand(time(NULL));

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		assert(false);
	}

	g_window = SDL_CreateWindow("Why Crystals ?",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_window_w, g_window_h, 0);
	assert(g_window != NULL);

	g_renderer = SDL_CreateRenderer(g_window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	assert(g_renderer != NULL);

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

	mg_w = g_window_w / tile_w;
	mg_h = g_window_h / tile_h;
	mg = malloc(sizeof(tile_t) * mg_w * mg_h);

	for (int y = 0; y < mg_h; y++)
	for (int x = 0; x < mg_w; x++)
	{
		tile_t* tile = &mg[y * mg_w + x];
		tile->c = rand() % 5 == 0 ? '#' : ' ';
	}

	player_tc = (tc_t){mg_w / 2, mg_h / 2};

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
						case SDLK_UP:
							player_tc.y--;
						break;
						case SDLK_RIGHT:
							player_tc.x++;
						break;
						case SDLK_DOWN:
							player_tc.y++;
						break;
						case SDLK_LEFT:
							player_tc.x--;
						break;
					}
				break;
			}
		}

		SDL_SetRenderDrawColor(g_renderer, cool_bg.r, cool_bg.g, cool_bg.b, 255);
		SDL_RenderClear(g_renderer);

		for (int y = 0; y < mg_h; y++)
		for (int x = 0; x < mg_w; x++)
		{
			SDL_Rect rect = {x * tile_w, y * tile_h, tile_w, tile_h};
			tile_t const* tile = &mg[y * mg_w + x];
			char c = tile->c;
			rgb_t c_color = cool_white;
			if (player_tc.x == x && player_tc.y == y)
			{
				c = '@';
				c_color = cool_yellow;
			}
			if (c == ' ')
			{
				SDL_SetRenderDrawColor(g_renderer, cool_bg.r, cool_bg.g, cool_bg.b, 255);
			}
			else
			{
				SDL_SetRenderDrawColor(g_renderer, cool_h.r, cool_h.g, cool_h.b, 255);
			}
			SDL_RenderFillRect(g_renderer, &rect);
			draw_char(c, c_color, rect);
		}

		SDL_RenderPresent(g_renderer);
	}

	TTF_Quit();
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}
