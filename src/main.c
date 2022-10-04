
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

enum obj_type_t
{
	OBJ_NONE,
	OBJ_PLAYER,
	OBJ_ROCK,
};
typedef enum obj_type_t obj_type_t;

struct obj_t
{
	obj_type_t type;
};
typedef struct obj_t obj_t;

struct tile_t
{
	bool is_path;
	obj_t obj;
};
typedef struct tile_t tile_t;

int tile_w = 30, tile_h = 30;

/* Map grid. */
tile_t* mg = NULL;
int mg_w = -1, mg_h = -1;

/* Tile coords. */
struct tc_t
{
	int x, y;
};
typedef struct tc_t tc_t;

bool tc_in_bounds(tc_t tc)
{
	return 0 <= tc.x && tc.x < mg_w && 0 <= tc.y && tc.y < mg_h;
}

tile_t* mg_tile(tc_t tc)
{
	if (tc_in_bounds(tc))
	{
		return &mg[tc.y * mg_w + tc.x];
	}
	else
	{
		return NULL;
	}
}

tc_t player_tc;

/* Represents a move on the grid rather than a tile. */
typedef tc_t tc_move_t;
#define TC_MOVE_UP    (tc_move_t){.x =  0, .y = -1}
#define TC_MOVE_RIGHT (tc_move_t){.x = +1, .y =  0}
#define TC_MOVE_DOWN  (tc_move_t){.x =  0, .y = +1}
#define TC_MOVE_LEFT  (tc_move_t){.x = -1, .y =  0}

tc_move_t rand_one_tile_move(void)
{
	return (tc_move_t[]){TC_MOVE_UP, TC_MOVE_RIGHT, TC_MOVE_DOWN, TC_MOVE_LEFT}[rand() % 4];
}

tc_t tc_add_move(tc_t tc, tc_move_t move)
{
	return (tc_t){.x = tc.x + move.x, .y = tc.y + move.y};
}

void player_try_move(tc_move_t move)
{
	tile_t* src_player_tile = mg_tile(player_tc);
	tc_t dst_player_tc = tc_add_move(player_tc, move);
	tile_t* dst_player_tile = mg_tile(dst_player_tc);
	if (dst_player_tile == NULL)
	{
		return;
	}

	if (dst_player_tile->obj.type == OBJ_ROCK)
	{
		return;
	}

	dst_player_tile->obj = src_player_tile->obj;
	player_tc = dst_player_tc;
	src_player_tile->obj = (obj_t){.type = OBJ_NONE};
}

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
	mg = malloc(mg_w * mg_h * sizeof(tile_t));

	for (int y = 0; y < mg_h; y++)
	for (int x = 0; x < mg_w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = mg_tile(tc);
		*tile = (tile_t){0};
		tile->obj.type = OBJ_NONE;
	}

	{
		tc_t tc = {mg_w / 2, mg_h / 2};
		while (tc_in_bounds(tc))
		{
			mg_tile(tc)->is_path = true;
			tc = tc_add_move(tc, rand_one_tile_move());
		}
	}

	for (int y = 0; y < mg_h; y++)
	for (int x = 0; x < mg_w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = mg_tile(tc);
		
		if (!tile->is_path && rand() % 5 == 0)
		{
			tile->obj.type = OBJ_ROCK;
		}
	}

	player_tc = (tc_t){mg_w / 2, mg_h / 2};
	mg_tile(player_tc)->obj = (obj_t){.type = OBJ_PLAYER};

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
							player_try_move(TC_MOVE_UP);
						break;
						case SDLK_RIGHT:
							player_try_move(TC_MOVE_RIGHT);
						break;
						case SDLK_DOWN:
							player_try_move(TC_MOVE_DOWN);
						break;
						case SDLK_LEFT:
							player_try_move(TC_MOVE_LEFT);
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
			char* text = " ";
			rgb_t text_color = cool_white;
			rgb_t bg_color = cool_bg;
			switch (tile->obj.type)
			{
				case OBJ_NONE:
					;
				break;
				case OBJ_ROCK:
					text = "#";
				break;
				case OBJ_PLAYER:
					text = "@";
					text_color = cool_yellow;
				break;
			}
			if (tile->is_path)
			{
				bg_color = cool_h;
			}
			SDL_SetRenderDrawColor(g_renderer, bg_color.r, bg_color.g, bg_color.b, 255);
			SDL_RenderFillRect(g_renderer, &rect);
			draw_text(text, text_color, rect);
		}

		SDL_RenderPresent(g_renderer);
	}

	TTF_Quit();
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}
