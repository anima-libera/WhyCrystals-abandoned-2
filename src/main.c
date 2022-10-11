
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

int g_window_w = 1200, g_window_h = 600;
SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;
TTF_Font* g_font = NULL;

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

rgb_t g_color_bg_shadow = {5, 30, 25};
rgb_t g_color_bg = {10, 40, 35};
rgb_t g_color_bg_bright = {20, 80, 70};
rgb_t g_color_white = {180, 220, 200};
rgb_t g_color_yellow = {230, 240, 40};
rgb_t g_color_red = {230, 40, 35};
rgb_t g_color_green = {10, 240, 45};
rgb_t g_color_light_green = {40, 240, 90};
rgb_t g_color_dark_green = {10, 160, 40};
rgb_t g_color_cyan = {20, 200, 180};

SDL_Texture* text_to_texture(char const* text, rgba_t color)
{
	SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
	SDL_Surface* surface = TTF_RenderText_Solid(g_font, text, sdl_color);
	assert(surface != NULL);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(texture != NULL);
	SDL_FreeSurface(surface);
	return texture;
}

void draw_text_stetch(char const* text, rgb_t color, SDL_Rect rect)
{
	SDL_Texture* texture = text_to_texture(text, rgb_to_rgba(color, 255));
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

void log_text(char* format, ...)
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
			rgb_to_rgba(g_color_white, 255));
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

void recompute_vision(void)
{
	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = mg_tile(tc);
		tile->vision = 0;
	}

	tc_t src_tc = loc_tc(get_obj(g_player_oid)->loc);

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = mg_tile(tc);
		if (tile->vision != 0)
		{
			continue;
		}
		
		int vision = 5;
		bresenham_it_t it = line_bresenham_init(src_tc, tc);
		while (line_bresenham_iter(&it))
		{
			tile_t* tile = mg_tile(it.head);
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
				switch (obj->type)
				{
					case OBJ_GRASS:
						vision -= 3;
					break;
					case OBJ_BUSH:
						vision -= 4;
					break;
					case OBJ_MOSS:
						vision -= 1;
					break;
					case OBJ_TREE:
						vision -= 8;
					break;
					case OBJ_ROCK:
						vision -= 100;
					break;
					case OBJ_CRYSTAL:
						vision -= 4;
					break;
					default:
						;
					break;
				}
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
		case OBJ_PLAYER:
			return "player";
		case OBJ_CRYSTAL:
			return "crystal";
		case OBJ_ROCK:
			return "rock";
		case OBJ_GRASS:
			return "grass";
		case OBJ_BUSH:
			return "bush";
		case OBJ_MOSS:
			return "moss";
		case OBJ_TREE:
			return "tree";
		case OBJ_SLIME:
			return "slime";
		default:
			assert(false);
	}
}

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
	bool event_visible = mg_tile(tc_attacker)->vision > 0 || mg_tile(tc_target)->vision > 0;
	
	obj_target->life--;
	if (obj_target->life <= 0)
	{
		obj_dealloc(oid_target);
		recompute_vision();
		if (event_visible)
		{
			log_text("A %s killed a %s.",
				obj_type_name(obj_attacker->type), obj_type_name(obj_target->type));
		}
	}
	else
	{
		if (obj_target->visual_effect.type != VISUAL_EFFECT_ATTACK)
		{
			obj_target->visual_effect.type = VISUAL_EFFECT_DAMAGED;
			obj_target->visual_effect.t = 0;
			obj_target->visual_effect.t_max = 20;
			obj_target->visual_effect.dir = dir;
		}
		if (event_visible)
		{
			log_text("A %s hit a %s.",
				obj_type_name(obj_attacker->type), obj_type_name(obj_target->type));
		}
	}
	obj_attacker->visual_effect.type = VISUAL_EFFECT_ATTACK;
	obj_attacker->visual_effect.t = 0;
	obj_attacker->visual_effect.t_max = 16;
	obj_attacker->visual_effect.dir = dir;
}

void obj_try_move(oid_t oid, tm_t move)
{
	tc_t dst_tc = tc_add_tm(loc_tc(get_obj(oid)->loc), move);
	tile_t* dst_tile = mg_tile(dst_tc);
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
		switch (obj_on_dst->type)
		{
			case OBJ_ROCK:
			case OBJ_CRYSTAL:
			case OBJ_TREE:
				return;
			case OBJ_BUSH:
			case OBJ_SLIME:
			case OBJ_PLAYER:
				obj_hits_obj(oid, oid_on_dst);
				return;
			default:
				;
			break;
		}
	}

	obj_move(oid, dst_tc);

	obj_t* obj = get_obj(oid);
	obj->visual_effect.type = VISUAL_EFFECT_MOVE;
	obj->visual_effect.t = 0;
	obj->visual_effect.t_max = 13;
	obj->visual_effect.dir = tm_reverse(move);
}

int interpolate(int progression, int progression_max, int value_min, int value_max)
{
	return
		(value_min * (progression_max - progression) + progression * value_max)
		/ progression_max;
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
		g_asset_font,
		g_asset_font_size);
	assert(rwops_font != NULL);
	g_font = TTF_OpenFontRW(rwops_font, 0, 20);
	assert(g_font != NULL);

	g_mg_rect.w = 60;
	g_mg_rect.h = 60;
	g_mg = malloc(g_mg_rect.w * g_mg_rect.h * sizeof(tile_t));

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = mg_tile(tc);
		*tile = (tile_t){0};
	}

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
			tile_t* tile = mg_tile(tc);
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
			tile_t* tile = mg_tile(tc);
			if (tile->is_path)
			{
				/* Making sure that the path only has straight lines and turns
				 * and does not contains T-shaped or plus-shaped parts. */
				int neighbor_path_count = 0;
				for (int i = 0; i < 4; i++)
				{
					tc_t neighbor_tc = tc_add_tm(tc, TM_ONE_ALL[i]);
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
	printf("Path generation try count: %d\n", path_try_count);

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = mg_tile(tc);

		if (tile->is_path)
		{
			continue;
		}

		bool neighbor_to_path = false;
		for (int i = 0; i < 4; i++)
		{
			tile_t* neighbor_tile = mg_tile(tc_add_tm(tc, TM_ONE_ALL[i]));
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
			!oid_da_contains_type(&tile->oid_da, OBJ_CRYSTAL) &&
			!oid_da_contains_type(&tile->oid_da, OBJ_PLAYER) &&
			rand() % 20 == 0)
		{
			oid_t oid = obj_alloc(OBJ_SLIME, tc_to_loc(tc));
			get_obj(oid)->life = 5;
		}
	}

	tc_t player_tc = {g_mg_rect.w / 2, g_mg_rect.h / 2};
	g_player_oid = obj_alloc(OBJ_PLAYER, tc_to_loc(player_tc));
	get_obj(g_player_oid)->life = 10;

	recompute_vision();
	
	float camera_x, camera_y;
	camera_x = ((float)g_mg_rect.w / 2.0f) * (float)g_tile_w + 0.5f * (float)g_tile_w;
	camera_y = ((float)g_mg_rect.h / 2.0f) * (float)g_tile_h + 0.5f * (float)g_tile_h;

	float iteration_time = 0.0f;

	bool running = true;
	while (running)
	{
		clock_t clock_start_loop = clock();

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
							g_tile_w += 5;
							g_tile_h += 5;
						break;
						case SDLK_KP_MINUS:
							g_tile_w -= 5;
							g_tile_h -= 5;
						break;
						case SDLK_UP:
							obj_try_move(g_player_oid, TM_UP);
							perform_turn = true;
						break;
						case SDLK_RIGHT:
							obj_try_move(g_player_oid, TM_RIGHT);
							perform_turn = true;
						break;
						case SDLK_DOWN:
							obj_try_move(g_player_oid, TM_DOWN);
							perform_turn = true;
						break;
						case SDLK_LEFT:
							obj_try_move(g_player_oid, TM_LEFT);
							perform_turn = true;
						break;
					}
				break;
			}
		}

		if (perform_turn)
		{
			oid_t oid = OID_NULL;
			while (oid_iter(&oid))
			{
				obj_t* obj = get_obj(oid);
				if (obj->type == OBJ_SLIME)
				{
					if (rand() % 3 == 0)
					{
						obj_try_move(oid, rand_tm_one());
					}
				}
			}

			recompute_vision();
			g_turn_number++;
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

			tile_t const* tile = mg_tile(tc);

			char* text = " ";
			int text_stretch = 0;
			rgb_t text_color = g_color_white;
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

			for (int i = 0; i < tile->oid_da.len; i++)
			{
				obj_t* obj = get_obj(tile->oid_da.arr[i]);
				if (obj == NULL)
				{
					continue;
				}
				if (obj->visual_effect.type != VISUAL_EFFECT_NONE)
				{
					if (obj->visual_effect.type == VISUAL_EFFECT_DAMAGED)
					{
						text_color = g_color_red;

						tc_t src = tc_add_tm(tc, obj->visual_effect.dir);
						SDL_Rect src_rect = {
							src.x * g_tile_w + g_window_w / 2 - (int)camera_x,
							src.y * g_tile_h + g_window_h / 2 - (int)camera_y,
							g_tile_w, g_tile_h};
						rect.x = interpolate(
							obj->visual_effect.t + 40, obj->visual_effect.t_max + 40,
							src_rect.x, rect.x);
						rect.y = interpolate(
							obj->visual_effect.t + 40, obj->visual_effect.t_max + 40,
							src_rect.y, rect.y);
					}
					else if (obj->visual_effect.type == VISUAL_EFFECT_MOVE ||
						obj->visual_effect.type == VISUAL_EFFECT_ATTACK)
					{
						tc_t src = tc_add_tm(tc, obj->visual_effect.dir);
						SDL_Rect src_rect = {
							src.x * g_tile_w + g_window_w / 2 - (int)camera_x,
							src.y * g_tile_h + g_window_h / 2 - (int)camera_y,
							g_tile_w, g_tile_h};
						rect.x = interpolate(
							obj->visual_effect.t, obj->visual_effect.t_max,
							src_rect.x, rect.x);
						rect.y = interpolate(
							obj->visual_effect.t, obj->visual_effect.t_max,
							src_rect.y, rect.y);
					}
					obj->visual_effect.t++;
					if (obj->visual_effect.t >= obj->visual_effect.t_max)
					{
						obj->visual_effect = (visual_effect_t){0};
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
				draw_text_stetch(text, text_color, rect);
			}
		}

		draw_log();

		{
			SDL_Rect rect = {0, g_window_h - 5, iteration_time * g_window_w, 5};
			SDL_SetRenderDrawColor(g_renderer,
				g_color_white.r, g_color_white.g, g_color_white.b, 255);
			SDL_RenderFillRect(g_renderer, &rect);
		}

		SDL_RenderPresent(g_renderer);

		clock_t clock_end_loop = clock();
		iteration_time = (float)(clock_end_loop - clock_start_loop) / (float)CLOCKS_PER_SEC;
	}

	TTF_Quit();
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}
