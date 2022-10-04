
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

rgb_t g_color_bg = {10, 40, 35};
rgb_t g_color_bg_bright = {20, 80, 70};
rgb_t g_color_white = {180, 220, 200};
rgb_t g_color_yellow = {230, 240, 40};
rgb_t g_color_red = {230, 40, 35};

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
	OBJ_CRYSTAL,
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

int g_tile_w = 30, g_tile_h = 30;

struct tc_rect_t
{
	int x, y, w, h;
};
typedef struct tc_rect_t tc_rect_t;

/* Map grid. */
tile_t* g_mg = NULL;
tc_rect_t g_mg_rect = {0, 0, -1, -1};

/* Tile coords. */
struct tc_t
{
	int x, y;
};
typedef struct tc_t tc_t;

bool tc_in_rect(tc_t tc, tc_rect_t rect)
{
	return
		rect.x <= tc.x && tc.x < rect.x + rect.w &&
		rect.y <= tc.y && tc.y < rect.y + rect.h;
}

tile_t* mg_tile(tc_t tc)
{
	if (tc_in_rect(tc, g_mg_rect))
	{
		return &g_mg[tc.y * g_mg_rect.w + tc.x];
	}
	else
	{
		return NULL;
	}
}

tc_t g_crystal_tc;
tc_t g_player_tc;

/* Tile move. Represents a move on the grid rather than a tile. */
typedef tc_t tm_t;
#define TM_UP    (tm_t){.x =  0, .y = -1}
#define TM_RIGHT (tm_t){.x = +1, .y =  0}
#define TM_DOWN  (tm_t){.x =  0, .y = +1}
#define TM_LEFT  (tm_t){.x = -1, .y =  0}
#define TM_ONE_ALL (tm_t[]){TM_UP, TM_RIGHT, TM_DOWN, TM_LEFT}

tm_t rand_tm_one(void)
{
	return TM_ONE_ALL[rand() % 4];
}

bool tm_one_orthogonal(tm_t a, tm_t b)
{
	return !((a.x != 0 && b.x != 0) || (a.y != 0 && b.y != 0));
}

tc_t tc_add_move(tc_t tc, tm_t move)
{
	return (tc_t){.x = tc.x + move.x, .y = tc.y + move.y};
}

void player_try_move(tm_t move)
{
	tile_t* src_player_tile = mg_tile(g_player_tc);
	tc_t dst_player_tc = tc_add_move(g_player_tc, move);
	tile_t* dst_player_tile = mg_tile(dst_player_tc);
	if (dst_player_tile == NULL)
	{
		return;
	}

	switch (dst_player_tile->obj.type)
	{
		case OBJ_ROCK:
		case OBJ_CRYSTAL:
			return;
		default:
			break;
	}

	dst_player_tile->obj = src_player_tile->obj;
	g_player_tc = dst_player_tc;
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

	g_mg_rect.w = g_window_w / g_tile_w;
	g_mg_rect.h = g_window_h / g_tile_h;
	g_mg = malloc(g_mg_rect.w * g_mg_rect.h * sizeof(tile_t));

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = mg_tile(tc);
		*tile = (tile_t){0};
		tile->obj.type = OBJ_NONE;
	}

	g_crystal_tc = (tc_t){
		.x = g_mg_rect.x + g_mg_rect.w / 4 + rand() % (g_mg_rect.w / 9),
		.y = g_mg_rect.y + g_mg_rect.h / 3 + rand() % (g_mg_rect.h / 3)};
	mg_tile(g_crystal_tc)->obj = (obj_t){.type = OBJ_CRYSTAL};

	/* Generate the path. */
	while (true)
	{
		/* Generate a path that may or may not be validated. */
		int margin = 3;
		tc_rect_t path_rect = {
			g_mg_rect.x + margin, g_mg_rect.y + margin,
			g_mg_rect.w - margin, g_mg_rect.h - 2 * margin};
		tc_t tc = g_crystal_tc;
		tm_t direction = rand_tm_one();
		int same_direction_steps = 0;
		while (tc_in_rect(tc, path_rect))
		{
			tile_t* tile = mg_tile(tc);
			tile->is_path = true;
			if (tc.x == g_mg_rect.w-1)
			{
				break;
			}
			tc = tc_add_move(tc, direction);
			same_direction_steps++;

			if (same_direction_steps >= 1 && rand() % (same_direction_steps == 1 ? 6 : 2) == 0)
			{
				/* Change the direction. */
				tm_t new_direction = rand_tm_one();
				while (!tm_one_orthogonal(direction, new_direction))
				{
					new_direction = rand_tm_one();
				}
				direction = new_direction;
				same_direction_steps = 0;
			}
		}

		/* Validate the generated path, or not. */
		for (int y = 0; y < g_mg_rect.h; y++)
		for (int x = 0; x < g_mg_rect.w; x++)
		{
			tc_t tc = {x, y};
			tile_t* tile = mg_tile(tc);
			if (tile->is_path)
			{
				/* Making sure that the path only has straight lines and turns
				 * and does not contains T-shaped or plus-shaped parts. */
				int neighbor_path_count = 0;
				for (int i = 0; i < 4; i++)
				{
					tc_t neighbor_tc = tc_add_move(tc, TM_ONE_ALL[i]);
					tile_t* neighbor_tile = mg_tile(neighbor_tc);
					if (neighbor_tile != NULL && neighbor_tile->is_path)
					{
						neighbor_path_count++;
					}
				}
				if (neighbor_path_count >= 3)
				{
					goto path_invalid;
				}				
			}
		}
		/* Making sure that the path touches the right part of the map. */
		bool path_touches_right_edge = false;
		for (int y = 0; y < g_mg_rect.h; y++)
		{
			tc_t tc = {g_mg_rect.w-1, y};
			tile_t* tile = mg_tile(tc);
			if (tile->is_path)
			{
				path_touches_right_edge = true;
				break;
			}
		}
		if (!path_touches_right_edge)
		{
			goto path_invalid;
		}

		/* The generated path was validated. */
		break;

		path_invalid:
		/* The generated path was not validated.
		 * It is erased before retrying. */
		for (int y = 0; y < g_mg_rect.h; y++)
		for (int x = 0; x < g_mg_rect.w; x++)
		{
			tc_t tc = {x, y};
			tile_t* tile = mg_tile(tc);
			tile->is_path = false;
		}
	}

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = mg_tile(tc);
		
		if (!tile->is_path && rand() % 5 == 0)
		{
			tile->obj.type = OBJ_ROCK;
		}
	}

	g_player_tc = (tc_t){g_mg_rect.w / 2, g_mg_rect.h / 2};
	mg_tile(g_player_tc)->obj = (obj_t){.type = OBJ_PLAYER};

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
							player_try_move(TM_UP);
						break;
						case SDLK_RIGHT:
							player_try_move(TM_RIGHT);
						break;
						case SDLK_DOWN:
							player_try_move(TM_DOWN);
						break;
						case SDLK_LEFT:
							player_try_move(TM_LEFT);
						break;
					}
				break;
			}
		}

		SDL_SetRenderDrawColor(g_renderer, g_color_bg.r, g_color_bg.g, g_color_bg.b, 255);
		SDL_RenderClear(g_renderer);

		for (int y = 0; y < g_mg_rect.h; y++)
		for (int x = 0; x < g_mg_rect.w; x++)
		{
			SDL_Rect rect = {x * g_tile_w, y * g_tile_h, g_tile_w, g_tile_h};
			tile_t const* tile = &g_mg[y * g_mg_rect.w + x];
			char* text = " ";
			rgb_t text_color = g_color_white;
			rgb_t bg_color = g_color_bg;
			switch (tile->obj.type)
			{
				case OBJ_NONE:
					;
				break;
				case OBJ_ROCK:
					text = "#";
				break;
				case OBJ_CRYSTAL:
					text = "A";
					text_color = g_color_red;
				break;
				case OBJ_PLAYER:
					text = "@";
					text_color = g_color_yellow;
				break;
			}
			if (tile->is_path)
			{
				bg_color = g_color_bg_bright;
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
