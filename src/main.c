
#include "embedded.h"
#include "utils.h"
#include "objects.h"
#include "mapgrid.h"
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* Time since the beginning of the game loop, in milliseconds. */
int g_game_duration = 0;

int g_window_w = 1200, g_window_h = 600;
SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;

enum font_t
{
	FONT_RG,
	FONT_TL,
	FONT_NUMBER
};
typedef enum font_t font_t;
TTF_Font* g_font_table[FONT_NUMBER] = {0};

struct rgb_t
{
	uint8_t r, g, b;
};
typedef struct rgb_t rgb_t;

struct rgba_t
{
	uint8_t r, g, b, a;
};
typedef struct rgba_t rgba_t;

rgba_t rgb_to_rgba(rgb_t rgb, uint8_t alpha)
{
	return (rgba_t){rgb.r, rgb.g, rgb.b, alpha};
}

rgb_t g_color_bg_shadow =   {  5,  30,  25};
rgb_t g_color_bg =          { 10,  40,  35};
rgb_t g_color_bg_bright =   { 20,  80,  70};
rgb_t g_color_white =       {180, 220, 200};
rgb_t g_color_yellow =      {230, 240,  40};
rgb_t g_color_red =         {230,  40,  35};
rgb_t g_color_green =       { 10, 240,  45};
rgb_t g_color_light_green = { 40, 240,  90};
rgb_t g_color_dark_green =  { 10, 160,  40};
rgb_t g_color_cyan =        { 20, 200, 180};

/* Screen coordinates. */
struct sc_t
{
	int x, y;
};
typedef struct sc_t sc_t;

SDL_Texture* text_to_texture(char const* text, rgba_t color, font_t font)
{
	assert(0 <= font && font < FONT_NUMBER);
	SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
	SDL_Surface* surface = TTF_RenderText_Solid(g_font_table[font], text, sdl_color);
	assert(surface != NULL);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(texture != NULL);
	SDL_FreeSurface(surface);
	return texture;
}

void draw_text_rect(char const* text, rgba_t color, font_t font, SDL_Rect rect)
{
	SDL_Texture* texture = text_to_texture(text, color, font);
	SDL_RenderCopy(g_renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
}

void draw_text_sc(char const* text, rgba_t color, font_t font, sc_t sc)
{
	SDL_Texture* texture = text_to_texture(text, color, font);
	SDL_Rect rect = {.x = sc.x, .y = sc.y};
	SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
	SDL_RenderCopy(g_renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
}

int g_turn_number = 0;

struct log_entry_t
{
	char* text;
	int turn_number;
	int time_remaining;
};
typedef struct log_entry_t log_entry_t;

int g_log_len = 0, g_log_cap = 0;
log_entry_t* g_log_da = NULL;

/* Returns an allocated string. */
char* format(char* format, ...)
{
	va_list va;
	va_start(va, format);
	int text_len = vsnprintf(NULL, 0, format, va);
	va_end(va);
	assert(text_len >= 0);
	char* text = malloc(text_len + 1);
	va_start(va, format);
	vsnprintf(text, text_len + 1, format, va);
	va_end(va);
	return text;
}

char* vformat(char* format, va_list va)
{
	va_list va_2;
	va_copy(va_2, va);
	int text_len = vsnprintf(NULL, 0, format, va);
	assert(text_len >= 0);
	char* text = malloc(text_len + 1);
	vsnprintf(text, text_len + 1, format, va_2);
	va_end(va_2);
	return text;
}

void log_text(char* format, ...)
{
	va_list va;
	va_start(va, format);
	char* text = vformat(format, va);
	va_end(va);

	DA_LENGTHEN(g_log_len += 1, g_log_cap, g_log_da, log_entry_t);
	for (int i = g_log_len-1; i > 0; i--)
	{
		g_log_da[i] = g_log_da[i-1];
	}
	g_log_da[0] = (log_entry_t){
		.text = text,
		.turn_number = g_turn_number,
		.time_remaining = 200};
}

void draw_log(void)
{
	int y = g_window_h - 30;
	for (int i = 0; i < g_log_len; i++)
	{
		int alpha = min(255, g_log_da[i].time_remaining * 6);

		SDL_Texture* texture = text_to_texture(g_log_da[i].text,
			rgb_to_rgba(g_color_white, 255), FONT_TL);
		SDL_SetTextureAlphaMod(texture, alpha);
		SDL_Rect rect = {.x = 10, .y = y};
		SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);

		SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, alpha / 3);
		SDL_RenderFillRect(g_renderer, &rect);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_RenderCopy(g_renderer, texture, NULL, &rect);
		SDL_DestroyTexture(texture);
		SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

		if (g_turn_number > g_log_da[i].turn_number + 16)
		{
			g_log_da[i].time_remaining--;
		}

		y -= rect.h;
	}

	if (g_log_len > 0 && g_log_da[g_log_len-1].time_remaining <= 0)
	{
		free(g_log_da[g_log_len-1].text);
		g_log_da[g_log_len-1] = (log_entry_t){0};
		g_log_len--;
	}
}

int g_tile_w = 30, g_tile_h = 30;

struct bresenham_it_t
{
	tc_t a, b, head;
	int dx, dy, i, d;
	bool just_initialized;
};
typedef struct bresenham_it_t bresenham_it_t;

bresenham_it_t line_bresenham_init(tc_t a, tc_t b)
{
	bresenham_it_t it;
	it.a = a;
	it.b = b;
	#define LINE_BRESENHAM_INIT(x_, y_) \
		it.d##x_ = abs(it.b.x_ - it.a.x_); \
		it.d##y_ = it.b.y_ - it.a.y_; \
		it.i = 1; \
		if (it.d##y_ < 0) \
		{ \
			it.i *= -1; \
			it.d##y_ *= -1; \
		} \
		it.d = (2 * it.d##y_) - it.d##x_; \
		it.head.x = it.a.x; \
		it.head.y = it.a.y;
	if (abs(b.y - a.y) < abs(b.x - a.x))
	{
		LINE_BRESENHAM_INIT(x, y)
	}
	else
	{
		LINE_BRESENHAM_INIT(y, x)
	}
	#undef LINE_BRESENHAM_INIT
	it.just_initialized = true;
	return it;
}

bool line_bresenham_iter(bresenham_it_t* it)
{
	if (it->just_initialized)
	{
		it->just_initialized = false;
		return true;
	}
	#define LINE_BRESENHAM_ITER(x_, y_) \
		if (it->head.x_ == it->b.x_) \
		{ \
			return false; \
		} \
		it->head.x_ += it->a.x_ < it->b.x_ ? 1 : -1; \
		if (it->d > 0) \
		{ \
			it->head.y_ += it->i; \
			it->d += (2 * (it->d##y_ - it->d##x_)); \
		} \
		else \
		{ \
			it->d += 2 * it->d##y_; \
		} \
		return true;
	if (abs(it->b.y - it->a.y) < abs(it->b.x - it->a.x))
	{
		LINE_BRESENHAM_ITER(x, y)
	}
	else
	{
		LINE_BRESENHAM_ITER(y, x)
	}
	#undef LINE_BRESENHAM_ITER
}

int obj_type_vision_blocking(obj_type_t type)
{
	switch (type)
	{
		case OBJ_GRASS:       return 3;
		case OBJ_BUSH:        return 4;
		case OBJ_MOSS:        return 1;
		case OBJ_TREE:        return 8;
		case OBJ_ROCK:        return 100;
		case OBJ_CRYSTAL:     return 4;
		case OBJ_SLIME:       return 2;
		case OBJ_CATERPILLAR: return 1;
		default:              return 1;
	}
}

void recompute_vision(void)
{
	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = get_tile(tc);
		tile->vision = 0;
	}

	tc_t src_tc = loc_tc(get_obj(g_player_oid)->loc);

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = get_tile(tc);
		if (tile->vision != 0)
		{
			continue;
		}
		
		int vision = 5;
		bresenham_it_t it = line_bresenham_init(src_tc, tc);
		while (line_bresenham_iter(&it))
		{
			tile_t* tile = get_tile(it.head);
			tile->vision = max(tile->vision, vision);
			if (tc_eq(it.head, src_tc))
			{
				continue;
			}
			for (int i = 0; i < tile->oid_da.len; i++)
			{
				obj_t* obj = get_obj(tile->oid_da.arr[i]);
				if (obj == NULL)
				{
					continue;
				}
				vision -= obj_type_vision_blocking(obj->type);
			}
			if (vision < 0)
			{
				vision = 0;
			}
		}
	}
}

char const* obj_type_name(obj_type_t type)
{
	switch (type)
	{
		case OBJ_PLAYER:      return "player";
		case OBJ_CRYSTAL:     return "crystal";
		case OBJ_ROCK:        return "rock";
		case OBJ_GRASS:       return "grass";
		case OBJ_BUSH:        return "bush";
		case OBJ_MOSS:        return "moss";
		case OBJ_TREE:        return "tree";
		case OBJ_SLIME:       return "slime";
		case OBJ_CATERPILLAR: return "caterpillar";
		default:              assert(false); exit(EXIT_FAILURE);
	}
}

bool g_game_over = false;
char* g_game_over_cause = NULL;

void obj_hits_obj(oid_t oid_attacker, oid_t oid_target)
{
	obj_t* obj_attacker = get_obj(oid_attacker);
	obj_t* obj_target = get_obj(oid_target);
	if (obj_target == NULL)
	{
		return;
	}
	tc_t tc_attacker = loc_tc(obj_attacker->loc);
	tc_t tc_target = loc_tc(obj_target->loc);
	tm_t dir = tc_diff_as_tm(tc_attacker, tc_target);
	bool event_visible = get_tile(tc_attacker)->vision > 0 || get_tile(tc_target)->vision > 0;
	
	obj_target->life--;
	if (obj_target->life <= 0)
	{
		if (event_visible)
		{
			log_text("A %s killed a %s.",
				obj_type_name(obj_attacker->type), obj_type_name(obj_target->type));
		}
		if (obj_target->type == OBJ_PLAYER)
		{
			log_text("Game over.");
			g_game_over = true;
			g_game_over_cause = format("Killed by a %s.", obj_type_name(obj_attacker->type));
		}
		obj_dealloc(oid_target);
	}
	else
	{
		visual_effect_da_add(&obj_target->visual_effect_da, (visual_effect_t){
			.type = VISUAL_EFFECT_DAMAGED,
			.t_begin = g_game_duration,
			.t_end = g_game_duration + 100,
			.dir = dir});
		if (event_visible)
		{
			log_text("A %s hit a %s.",
				obj_type_name(obj_attacker->type), obj_type_name(obj_target->type));
		}
	}
	visual_effect_da_add(&obj_attacker->visual_effect_da, (visual_effect_t){
		.type = VISUAL_EFFECT_ATTACK,
		.t_begin = g_game_duration,
		.t_end = g_game_duration + 80,
		.dir = dir});
}

bool obj_type_is_blocking(obj_type_t type)
{
	switch (type)
	{
		case OBJ_TREE:
		case OBJ_ROCK:
		case OBJ_CRYSTAL:
		case OBJ_BUSH:
		case OBJ_SLIME:
		case OBJ_CATERPILLAR:
		case OBJ_PLAYER:
			return true;
		case OBJ_GRASS:
		case OBJ_MOSS:
		default:
			return false;
	}
}

bool obj_type_can_get_hit_for_now(obj_type_t type)
{
	switch (type)
	{
		case OBJ_BUSH:
		case OBJ_SLIME:
		case OBJ_CATERPILLAR:
		case OBJ_PLAYER:
			return true;
		case OBJ_TREE:
		case OBJ_ROCK:
		case OBJ_CRYSTAL:
		case OBJ_GRASS:
		case OBJ_MOSS:
		default:
			return false;
	}
}

void obj_try_move(oid_t oid, tm_t move)
{
	assert(get_obj(oid) != NULL);
	tc_t dst_tc = tc_add_tm(loc_tc(get_obj(oid)->loc), move);
	tile_t* dst_tile = get_tile(dst_tc);
	if (dst_tile == NULL)
	{
		return;
	}

	for (int i = 0; i < dst_tile->oid_da.len; i++)
	{
		oid_t oid_on_dst = dst_tile->oid_da.arr[i];
		obj_t* obj_on_dst = get_obj(oid_on_dst);
		if (obj_on_dst == NULL)
		{
			continue;
		}
		else if (obj_type_can_get_hit_for_now(obj_on_dst->type))
		{
			obj_hits_obj(oid, oid_on_dst);
			return;
		}
		else if (obj_type_is_blocking(obj_on_dst->type))
		{
			return;
		}
	}

	obj_move(oid, dst_tc);

	visual_effect_da_add(&get_obj(oid)->visual_effect_da, (visual_effect_t){
		.type = VISUAL_EFFECT_MOVE,
		.t_begin = g_game_duration,
		.t_end = g_game_duration + 60,
		.dir = tm_reverse(move)});
}

void generate_map(void)
{
	tc_t crystal_tc = {
		.x = g_mg_rect.x + g_mg_rect.w / 4 + rand() % (g_mg_rect.w / 9),
		.y = g_mg_rect.y + g_mg_rect.h / 3 + rand() % (g_mg_rect.h / 3)};
	g_crystal_oid = obj_alloc(OBJ_CRYSTAL, tc_to_loc(crystal_tc));

	/* Generate the path. */
	int path_try_count = 0;
	while (true)
	{
		path_try_count++;

		/* Generate a path that may or may not be validated. */
		int margin = 3;
		tc_rect_t path_rect = {
			g_mg_rect.x + margin, g_mg_rect.y + margin,
			g_mg_rect.w - margin, g_mg_rect.h - 2 * margin};
		tc_t tc = crystal_tc;
		tm_t direction = rand_tm_one();
		int same_direction_steps = 0;
		while (tc_in_rect(tc, path_rect))
		{
			tile_t* tile = get_tile(tc);
			tile->is_path = true;
			if (tc.x == g_mg_rect.w-1)
			{
				break;
			}
			tc = tc_add_tm(tc, direction);
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
			tile_t* tile = get_tile(tc);
			if (tile->is_path)
			{
				/* Making sure that the path only has straight lines and turns
				 * and does not contains T-shaped or plus-shaped parts. */
				int neighbor_path_count = 0;
				for (int i = 0; i < 4; i++)
				{
					tc_t neighbor_tc = tc_add_tm(tc, TM_ONE_ALL[i]);
					tile_t* neighbor_tile = get_tile(neighbor_tc);
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
			tile_t* tile = get_tile(tc);
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
			tile_t* tile = get_tile(tc);
			tile->is_path = false;
		}
	}
	printf("Path generation try count: %d\n", path_try_count);

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = get_tile(tc);

		if (tile->is_path)
		{
			continue;
		}

		bool neighbor_to_path = false;
		for (int i = 0; i < 4; i++)
		{
			tile_t* neighbor_tile = get_tile(tc_add_tm(tc, TM_ONE_ALL[i]));
			if (neighbor_tile != NULL && neighbor_tile->is_path)
			{
				neighbor_to_path = true;
				break;
			}
		}
		
		if (rand() % 5 == 0)
		{
			oid_t oid = obj_alloc(OBJ_ROCK, tc_to_loc(tc));
			get_obj(oid)->life = 1000;
		}
		else if (rand() % (neighbor_to_path ? 2 : 5) == 0)
		{
			oid_t oid = obj_alloc(OBJ_TREE, tc_to_loc(tc));
			get_obj(oid)->life = 100;
		}
		else if (rand() % 3 != 0)
		{
			oid_t oid = obj_alloc(OBJ_GRASS, tc_to_loc(tc));
			get_obj(oid)->life = 4;
		}
		else if (rand() % 3 != 0)
		{
			oid_t oid = obj_alloc(OBJ_MOSS, tc_to_loc(tc));
			get_obj(oid)->life = 2;
		}
		else if (rand() % 3 != 0)
		{
			oid_t oid = obj_alloc(OBJ_BUSH, tc_to_loc(tc));
			get_obj(oid)->life = 8;

			oid = obj_alloc(OBJ_MOSS, tc_to_loc(tc));
			get_obj(oid)->life = 2;
		}

		if (!oid_da_contains_type(&tile->oid_da, OBJ_ROCK) &&
			!oid_da_contains_type(&tile->oid_da, OBJ_TREE) &&
			!oid_da_contains_type(&tile->oid_da, OBJ_CRYSTAL))
		{
			if (rand() % 20 == 0)
			{
				oid_t oid = obj_alloc(OBJ_SLIME, tc_to_loc(tc));
				get_obj(oid)->life = 5;
			}
			else if (rand() % 50 == 0)
			{
				oid_t oid = obj_alloc(OBJ_CATERPILLAR, tc_to_loc(tc));
				get_obj(oid)->life = 6;
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

	g_window = SDL_CreateWindow("Why Crystals ?",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_window_w, g_window_h,
		SDL_WINDOW_RESIZABLE);
	assert(g_window != NULL);

	g_renderer = SDL_CreateRenderer(g_window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	assert(g_renderer != NULL);

	if (TTF_Init() < 0)
	{
		assert(false);
	}

	SDL_RWops* rwops_font = SDL_RWFromConstMem(
		g_asset_font_1,
		g_asset_font_1_size);
	assert(rwops_font != NULL);
	g_font_table[0] = TTF_OpenFontRW(rwops_font, 0, 20);
	assert(g_font_table[0] != NULL);

	rwops_font = SDL_RWFromConstMem(
		g_asset_font_2,
		g_asset_font_2_size);
	assert(rwops_font != NULL);
	g_font_table[1] = TTF_OpenFontRW(rwops_font, 0, 30);
	assert(g_font_table[1] != NULL);

	g_mg_rect.w = 60;
	g_mg_rect.h = 60;
	g_mg = malloc(g_mg_rect.w * g_mg_rect.h * sizeof(tile_t));

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = get_tile(tc);
		*tile = (tile_t){0};
	}

	generate_map();

	{
		/* Place the player on a tile that does not contains blocking objects. */
		tc_t tc = {g_mg_rect.w / 2, g_mg_rect.h / 2};
		while (oid_da_contains_type_f(&get_tile(tc)->oid_da, obj_type_is_blocking))
		{
			tc_t new_tc = tc_add_tm(tc, rand_tm_one());
			while (get_tile(new_tc) == NULL)
			{
				new_tc = tc_add_tm(tc, rand_tm_one());
			}
			tc = new_tc;
		}
		g_player_oid = obj_alloc(OBJ_PLAYER, tc_to_loc(tc));
		get_obj(g_player_oid)->life = 10;
	}

	recompute_vision();
	
	float camera_x, camera_y;
	camera_x = ((float)g_mg_rect.w / 2.0f) * (float)g_tile_w + 0.5f * (float)g_tile_w;
	camera_y = ((float)g_mg_rect.h / 2.0f) * (float)g_tile_h + 0.5f * (float)g_tile_h;

	int obj_count = 0;

	int start_loop_time = SDL_GetTicks();
	g_game_duration = 0;
	int iteration_duration = 0; /* In milliseconds. */

	bool running = true;
	while (running)
	{
		int start_iteration_time = SDL_GetTicks();
		g_game_duration = start_iteration_time - start_loop_time;

		bool perform_turn = false;

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					running = false;
				break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_RESIZED:
							SDL_GetRendererOutputSize(g_renderer, &g_window_w, &g_window_h);
						break;
					}
				break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							running = false;
						break;
						case SDLK_KP_PLUS:
						case SDLK_KP_MINUS:
							g_tile_w += 5 * (event.key.keysym.sym == SDLK_KP_PLUS ? 1 : -1);
							g_tile_h += 5 * (event.key.keysym.sym == SDLK_KP_PLUS ? 1 : -1);
						break;
						case SDLK_UP:
						case SDLK_RIGHT:
						case SDLK_DOWN:
						case SDLK_LEFT:
							if (!g_game_over)
							{
								tm_t dir =
									event.key.keysym.sym == SDLK_UP ? TM_UP :
									event.key.keysym.sym == SDLK_RIGHT ? TM_RIGHT :
									event.key.keysym.sym == SDLK_DOWN ? TM_DOWN :
									event.key.keysym.sym == SDLK_LEFT ? TM_LEFT :
									(assert(false), (tm_t){0});
								obj_try_move(g_player_oid, dir);
								perform_turn = true;
							}
						break;
					}
				break;
			}
		}

		if (perform_turn)
		{
			obj_count = 0;
			oid_t oid = OID_NULL;
			while (oid_iter(&oid))
			{
				obj_count++;
				obj_t* obj = get_obj(oid);

				if (obj->type == OBJ_SLIME)
				{
					if (rand() % 3 == 0)
					{
						obj_try_move(oid, rand_tm_one());
					}
				}
				else if (obj->type == OBJ_CATERPILLAR)
				{
					for (int i = 0; i < 4; i++)
					{
						tm_t tm = TM_ONE_ALL[i];
						tc_t dst_tc = tc_add_tm(loc_tc(obj->loc), tm);
						tile_t* dst_tile = get_tile(dst_tc);
						if (dst_tile == NULL)
						{
							continue;
						}
						if (oid_da_contains_type(&dst_tile->oid_da, OBJ_PLAYER))
						{
							obj_try_move(oid, tm);
							goto caterpillar_done;
						}
					}
					obj_try_move(oid, rand_tm_one());
					caterpillar_done:;
				}
			}

			if (!g_game_over && get_obj(g_player_oid)->life <= 0)
			{
				log_text("Game over.");
				g_game_over = true;
				g_game_over_cause = format("Killed by.. something?");
			}
			else
			{
				recompute_vision();
				g_turn_number++;
			}
		}

		/* Move the camera (smoothly) to keep the player centered. */
		{
			tc_t player_tc = loc_tc(get_obj(g_player_oid)->loc);
			float const target_camera_x =
				(float)player_tc.x * (float)g_tile_w + 0.5f * (float)g_tile_w;
			float const target_camera_y =
				(float)player_tc.y * (float)g_tile_h + 0.5f * (float)g_tile_h;
			float const camera_speed = 0.035f;
			camera_x += (target_camera_x - camera_x) * camera_speed;
			camera_y += (target_camera_y - camera_y) * camera_speed;
		}

		SDL_SetRenderDrawColor(g_renderer,
			g_color_bg_shadow.r, g_color_bg_shadow.g, g_color_bg_shadow.b, 255);
		SDL_RenderClear(g_renderer);

		for (int pass = 0; pass < 2; pass++)
		for (int y = 0; y < g_mg_rect.h; y++)
		for (int x = 0; x < g_mg_rect.w; x++)
		{
			tc_t tc = {x, y};
			SDL_Rect base_rect = {
				x * g_tile_w + g_window_w / 2 - (int)camera_x,
				y * g_tile_h + g_window_h / 2 - (int)camera_y,
				g_tile_w, g_tile_h};
			SDL_Rect rect = {base_rect.x, base_rect.y, base_rect.w, base_rect.h};

			tile_t const* tile = get_tile(tc);

			char* text = " ";
			int text_stretch = 0;
			rgb_t text_color = g_color_white;
			font_t text_font = FONT_RG;
			rgb_t bg_color = g_color_bg;
			if (oid_da_contains_type(&tile->oid_da, OBJ_PLAYER))
			{
				text = "@";
				text_color = g_color_yellow;
			}
			else if (oid_da_contains_type(&tile->oid_da, OBJ_CRYSTAL))
			{
				text = "A";
				text_color = g_color_red;
			}
			else if (oid_da_contains_type(&tile->oid_da, OBJ_ROCK))
			{
				text = "#";
				text_color = g_color_white;
			}
			else if (oid_da_contains_type(&tile->oid_da, OBJ_TREE))
			{
				text = "Y";
				text_color = g_color_light_green;
			}
			else if (oid_da_contains_type(&tile->oid_da, OBJ_BUSH))
			{
				text = "n";
				text_stretch = 10;
				text_color = g_color_green;
			}
			else if (oid_da_contains_type(&tile->oid_da, OBJ_SLIME))
			{
				text = "o";
				text_color = g_color_cyan;
			}
			else if (oid_da_contains_type(&tile->oid_da, OBJ_CATERPILLAR))
			{
				text = "~";
				text_color = g_color_yellow;
			}
			else if (oid_da_contains_type(&tile->oid_da, OBJ_GRASS))
			{
				text = " v ";
				text_color = g_color_dark_green;
			}
			else if (oid_da_contains_type(&tile->oid_da, OBJ_MOSS))
			{
				text = " .. ";
				text_color = g_color_dark_green;
			}
			
			if (tile->is_path)
			{
				bg_color = g_color_bg_bright;
			}

			if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LALT])
			{
				if (tile->vision > 0)
				{
					bg_color = (rgb_t){
						min(255, tile->vision * 30),
						max(0, min(255, tile->vision * 30 - 255)),
						0};
				}
				else
				{
					bg_color = (rgb_t){0, 0, 0};
				}
			}
			if (tile->vision <= 0)
			{
				continue;
			}

			for (int i_obj = 0; i_obj < tile->oid_da.len; i_obj++)
			{
				obj_t* obj = get_obj(tile->oid_da.arr[i_obj]);
				if (obj == NULL)
				{
					continue;
				}
				for (int i = 0; i < obj->visual_effect_da.len; i++)
				{
					visual_effect_t* ve = &obj->visual_effect_da.arr[i];
					if (g_game_duration > ve->t_end)
					{
						visual_effect_da_remove(&obj->visual_effect_da, i);
						continue;
					}
					if (ve->type == VISUAL_EFFECT_NONE)
					{
						continue;
					}
					if (ve->type != VISUAL_EFFECT_NONE)
					{
						int t = g_game_duration - ve->t_begin;
						int t_max = ve->t_end - ve->t_begin;

						if (ve->type == VISUAL_EFFECT_DAMAGED)
						{
							text_color = g_color_red;

							tc_t src = tc_add_tm(tc, ve->dir);
							SDL_Rect src_rect = {
								src.x * g_tile_w + g_window_w / 2 - (int)camera_x,
								src.y * g_tile_h + g_window_h / 2 - (int)camera_y,
								g_tile_w, g_tile_h};
							rect.x = interpolate(t + 40, t_max + 40, src_rect.x, rect.x);
							rect.y = interpolate(t + 40, t_max + 40, src_rect.y, rect.y);
						}
						else if (ve->type == VISUAL_EFFECT_MOVE ||
							ve->type == VISUAL_EFFECT_ATTACK)
						{
							tc_t src = tc_add_tm(tc, ve->dir);
							SDL_Rect src_rect = {
								src.x * g_tile_w + g_window_w / 2 - (int)camera_x,
								src.y * g_tile_h + g_window_h / 2 - (int)camera_y,
								g_tile_w, g_tile_h};
							rect.x = interpolate(t, t_max, src_rect.x, rect.x);
							rect.y = interpolate(t, t_max, src_rect.y, rect.y);
						}
					}
				}
			}

			if (pass == 0)
			{
				SDL_SetRenderDrawColor(g_renderer, bg_color.r, bg_color.g, bg_color.b, 255);
				SDL_RenderFillRect(g_renderer, &base_rect);
			}
			else
			{
				rect.y -= 5 + text_stretch;
				rect.h += 10 + text_stretch + text_stretch / 3;
				draw_text_rect(text, rgb_to_rgba(text_color, 255), text_font, rect);
			}
		}

		/* Display some information in a corner. */
		{
			int y = 10;

			{
				char* text = format("HP: %d", get_obj(g_player_oid)->life);
				draw_text_sc(text,
					rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
				free(text);
				y += 30;
			}

			if (g_game_over)
			{
				draw_text_sc(g_game_over_cause,
					rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
				y += 30;
			}
			
			{
				char* text = format("FPS: %.0f",
					iteration_duration > 0.0f ?  1.0f / iteration_duration : 0.0f);
				draw_text_sc(text,
					rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
				free(text);
				y += 30;
			}

			{
				char* text = format("Time: %.1f s", (float)g_game_duration / 1000.0f);
				draw_text_sc(text,
					rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
				free(text);
				y += 30;
			}

			{
				char* text = format("Obj count: %d", obj_count);
				draw_text_sc(text,
					rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
				free(text);
				y += 30;
			}
		}

		draw_log();

		SDL_RenderPresent(g_renderer);

		int end_iteration_time = SDL_GetTicks();
		iteration_duration = end_iteration_time - start_iteration_time;
	}

	TTF_Quit();
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}
