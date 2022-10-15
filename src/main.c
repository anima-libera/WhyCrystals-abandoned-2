
#include "embedded.h"
#include "utils.h"
#include "objects.h"
#include "mapgrid.h"
#include "rendering.h"
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

int g_turn_number = 0;

struct log_entry_t
{
	/* Can be NULL, in which case the entry is just a separator. */
	char* text;
	/* The number of the turn during which this entry was logged.
	 * This is useful to make the entry disappear after a few turns. */
	int turn_number;
	/* When the entry is to disappear, it is given a countdown for a fading effect. */
	int time_remaining;
	#define LOG_ENTRY_TIME_REMAINING_INIT 60
	/* This is the time at which the entry was logged, for an appearing effect. */
	int time_created;
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
		.time_remaining = LOG_ENTRY_TIME_REMAINING_INIT,
		.time_created = SDL_GetTicks()};
}

void log_seperator(void)
{
	DA_LENGTHEN(g_log_len += 1, g_log_cap, g_log_da, log_entry_t);
	for (int i = g_log_len-1; i > 0; i--)
	{
		g_log_da[i] = g_log_da[i-1];
	}
	g_log_da[0] = (log_entry_t){
		.text = NULL,
		.turn_number = g_turn_number,
		.time_remaining = LOG_ENTRY_TIME_REMAINING_INIT,
		.time_created = SDL_GetTicks()};
}

void draw_log(void)
{
	/* Not drawing the leading separators (if any) and the redundent separators
	 * is more pleasing to the eyes, and it is easier than not producing these separators. */
	bool last_was_not_text = true;

	int y = g_window_h - 30;
	for (int i = 0; i < g_log_len; i++)
	{
		int alpha = min(255, g_log_da[i].time_remaining * (255 / LOG_ENTRY_TIME_REMAINING_INIT));
		int time_existing = SDL_GetTicks() - g_log_da[i].time_created;

		int height = min(5, time_existing);
		if (g_log_da[i].text != NULL)
		{
			SDL_Texture* texture = text_to_texture(g_log_da[i].text,
				rgb_to_rgba(g_color_white, 255), FONT_TL);
			SDL_SetTextureAlphaMod(texture, alpha);
			SDL_Rect rect = {.x = 10};
			SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
			rect.h = min(rect.h, time_existing / 3);
			rect.y = y - rect.h;
			height = rect.h;

			SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, alpha / 3);
			SDL_RenderFillRect(g_renderer, &rect);
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
			SDL_RenderCopy(g_renderer, texture, NULL, &rect);
			SDL_DestroyTexture(texture);
			SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
		}

		if (g_turn_number > g_log_da[i].turn_number + 16)
		{
			g_log_da[i].time_remaining--;
		}

		if (g_log_da[i].text != NULL || !last_was_not_text)
		{
			y -= height;
		}

		last_was_not_text = g_log_da[i].text == NULL;
	}

	while (g_log_len > 0 && g_log_da[g_log_len-1].time_remaining <= 0)
	{
		free(g_log_da[g_log_len-1].text);
		g_log_len--;
	}
}

void recompute_vision(void)
{
	obj_t* player_obj = get_obj(g_player_oid);
	if (player_obj == NULL)
	{
		return;
	}

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = get_tile(tc);
		tile->vision = 0;
	}

	tc_t src_tc = loc_tc(player_obj->loc);

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
				oid_t oid = tile->oid_da.arr[i];
				if (get_obj(oid) == NULL)
				{
					continue;
				}
				vision -= obj_vision_blocking(oid);
			}
			if (vision < 0)
			{
				vision = 0;
			}
		}
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
				obj_name(oid_attacker), obj_name(oid_target));
		}
		if (oid_eq(oid_target, g_player_oid))
		{
			log_text("Game over.");
			g_game_over = true;
			g_game_over_cause = format("Killed by a %s.", obj_name(oid_attacker));
		}
		obj_destroy(oid_target);
	}
	else
	{
		visual_effect_obj_da_add(&obj_target->visual_effect_da, (visual_effect_obj_t){
			.type = VISUAL_EFFECT_OBJ_DAMAGED,
			.time_begin = g_game_duration,
			.time_end = g_game_duration + 100,
			.dir = dir});
		if (event_visible)
		{
			log_text("A %s hit a %s.",
				obj_name(oid_attacker), obj_name(oid_target));
		}
	}
	visual_effect_obj_da_add(&obj_attacker->visual_effect_da, (visual_effect_obj_t){
		.type = VISUAL_EFFECT_OBJ_ATTACK,
		.time_begin = g_game_duration,
		.time_end = g_game_duration + 80,
		.dir = dir});
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
		else if (obj_can_get_hit_for_now(oid_on_dst))
		{
			obj_hits_obj(oid, oid_on_dst);
			return;
		}
		else if (obj_is_blocking(oid) && obj_is_blocking(oid_on_dst))
		{
			return;
		}
	}

	obj_change_loc(oid, tc_to_loc(dst_tc));

	visual_effect_obj_da_add(&get_obj(oid)->visual_effect_da, (visual_effect_obj_t){
		.type = VISUAL_EFFECT_OBJ_MOVE,
		.time_begin = g_game_duration,
		.time_end = g_game_duration + 60,
		.dir = tm_reverse(move)});
}

void generate_map(void)
{
	tc_t crystal_tc = {
		.x = g_mg_rect.x + g_mg_rect.w / 4 + rand() % (g_mg_rect.w / 9),
		.y = g_mg_rect.y + g_mg_rect.h / 3 + rand() % (g_mg_rect.h / 3)};
	obj_create(OBJ_CRYSTAL, tc_to_loc(crystal_tc));

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
			oid_t oid = obj_create(OBJ_ROCK, tc_to_loc(tc));
			get_obj(oid)->life = 1000;
		}
		else if (rand() % (neighbor_to_path ? 2 : 5) == 0)
		{
			oid_t oid = obj_create(OBJ_TREE, tc_to_loc(tc));
			get_obj(oid)->life = 100;
		}
		else if (rand() % 3 != 0)
		{
			oid_t oid = obj_create(OBJ_GRASS, tc_to_loc(tc));
			get_obj(oid)->life = 4;
		}
		else if (rand() % 3 != 0)
		{
			oid_t oid = obj_create(OBJ_MOSS, tc_to_loc(tc));
			get_obj(oid)->life = 2;
		}
		else if (rand() % 3 != 0)
		{
			oid_t oid = obj_create(OBJ_BUSH, tc_to_loc(tc));
			get_obj(oid)->life = 8;

			oid = obj_create(OBJ_MOSS, tc_to_loc(tc));
			get_obj(oid)->life = 2;
		}

		if (!oid_da_contains_type(&tile->oid_da, OBJ_ROCK) &&
			!oid_da_contains_type(&tile->oid_da, OBJ_TREE) &&
			!oid_da_contains_type(&tile->oid_da, OBJ_CRYSTAL))
		{
			if (rand() % 20 == 0)
			{
				oid_t slime_oid = obj_create(OBJ_SLIME, tc_to_loc(tc));
				get_obj(slime_oid)->life = 5;
				if (rand() % 2 == 0)
				{
					oid_t content_oid = obj_create(OBJ_CATERPILLAR, inside_obj_loc(slime_oid));
					get_obj(content_oid)->life = 1;
				}
			}
			else if (rand() % 50 == 0)
			{
				oid_t oid = obj_create(OBJ_CATERPILLAR, tc_to_loc(tc));
				get_obj(oid)->life = 6;
			}
		}
	}
}

void draw_viewed_tiles(camera_t camera)
{
	/* Pass 0 draws the background of all the viewed tiles,
	 * then pass 1 draws the foreground of all the viewed tiles. */
	for (int pass = 0; pass < 2; pass++)
	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t const* tile = get_tile(tc);
		if (tile->vision <= 0)
		{
			continue;
		}

		SDL_Rect base_rect = camera_tc_rect(camera, tc);
		SDL_Rect rect = {base_rect.x, base_rect.y, base_rect.w, base_rect.h};

		/* The tile may contain multiple objects.
		 * One is to be chosen to be drawn (and the others ignored). */
		oid_t oid = OID_NULL;
		obj_type_t type_priority[] = {
			OBJ_PLAYER,
			OBJ_CRYSTAL,
			OBJ_ROCK,
			OBJ_TREE,
			OBJ_BUSH,
			OBJ_SLIME,
			OBJ_CATERPILLAR,
			OBJ_GRASS,
			OBJ_MOSS};
		_Static_assert(sizeof type_priority / sizeof type_priority[0] == OBJ_TYPE_NUMBER,
			"Some object types have not been added to the drawing priority list.");
		for (int i = 0; i < (int)(sizeof type_priority / sizeof type_priority[0]); i++)
		{
			oid = oid_da_find_type(&tile->oid_da, type_priority[i]);
			if (get_obj(oid) == NULL)
			{
				oid = OID_NULL;
			}
			if (!oid_eq(oid, OID_NULL))
			{
				break;
			}
		}

		char const* text = " ";
		int text_stretch = 0;
		rgb_t text_color = g_color_white;
		font_t text_font = FONT_RG;
		rgb_t bg_color = g_color_bg;

		if (!oid_eq(oid, OID_NULL))
		{
			text = obj_text_representation(oid);
			text_stretch = obj_text_representation_stretch(oid);
			text_color = obj_foreground_color(oid);
			bg_color = obj_background_color(oid);
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

		obj_t* obj = get_obj(oid);
		if (obj != NULL)
		{
			for (int i = 0; i < obj->visual_effect_da.len; i++)
			{
				visual_effect_obj_t* ve = &obj->visual_effect_da.arr[i];
				if (g_game_duration > ve->time_end)
				{
					visual_effect_obj_da_remove(&obj->visual_effect_da, i);
					continue;
				}
				if (ve->type == VISUAL_EFFECT_OBJ_NONE)
				{
					continue;
				}
				if (ve->type != VISUAL_EFFECT_OBJ_NONE)
				{
					int t = g_game_duration - ve->time_begin;
					int t_max = ve->time_end - ve->time_begin;

					if (ve->type == VISUAL_EFFECT_OBJ_DAMAGED)
					{
						text_color = g_color_red;

						tc_t src = tc_add_tm(tc, ve->dir);
						SDL_Rect src_rect = camera_tc_rect(camera, src);
						rect.x = interpolate(t + 40, t_max + 40, src_rect.x, rect.x);
						rect.y = interpolate(t + 40, t_max + 40, src_rect.y, rect.y);
					}
					else if (ve->type == VISUAL_EFFECT_OBJ_MOVE ||
						ve->type == VISUAL_EFFECT_OBJ_ATTACK)
					{
						tc_t src = tc_add_tm(tc, ve->dir);
						SDL_Rect src_rect = camera_tc_rect(camera, src);
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
		while (oid_da_contains_obj_f(&get_tile(tc)->oid_da, obj_is_blocking))
		{
			tc_t new_tc = tc_add_tm(tc, rand_tm_one());
			while (get_tile(new_tc) == NULL)
			{
				new_tc = tc_add_tm(tc, rand_tm_one());
			}
			tc = new_tc;
		}
		g_player_oid = obj_create(OBJ_PLAYER, tc_to_loc(tc));
		get_obj(g_player_oid)->life = 10;
	}

	recompute_vision();
	
	camera_t camera;
	camera_set(&camera, loc_tc(get_obj(g_player_oid)->loc));

	int obj_count = 0;

	int start_loop_time = SDL_GetTicks();
	g_game_duration = 0;
	int iteration_duration = 0; /* In milliseconds. */
	int iteration_number = 0;
	int fps_in_some_recent_iteration = 0;

	bool running = true;
	while (running)
	{
		int start_iteration_time = SDL_GetTicks();
		g_game_duration = start_iteration_time - start_loop_time;

		bool must_perform_turn = false;

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
						case SDLK_s:
							/* Sucide key (for debugging purposes). */
							obj_destroy(g_player_oid);
							log_text("Game over.");
							g_game_over = true;
							g_game_over_cause = format("Killed by suicide xd.");
						break;
						case SDLK_p:
							/* Possess key (for debugging purposes). */
							while (true)
							{
								oid_t oid = rand_oid();
								obj_t* obj = get_obj(oid);
								if (obj->loc.type == LOC_TILE)
								{
									if (rand() % 30 != 0)
									{
										continue;
									}
									g_player_oid = oid;
									must_perform_turn = true;
									log_text("Possessing somthing somewhere.");
									break;
								}
							}
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
								tm_t dir = tm_from_arrow_key(event.key.keysym.sym);
								obj_try_move(g_player_oid, dir);
								must_perform_turn = true;
							}
						break;
					}
				break;
			}
		}

		if (must_perform_turn)
		{
			obj_count = 0;
			oid_t oid = OID_NULL;
			while (oid_iter(&oid))
			{
				obj_count++;
				obj_t* obj = get_obj(oid);

				if (obj->type == OBJ_SLIME)
				{
					if (obj->loc.type != LOC_TILE)
					{
						continue;
					}
					if (rand() % 3 == 0)
					{
						obj_try_move(oid, rand_tm_one());
					}
				}
				else if (obj->type == OBJ_CATERPILLAR)
				{
					if (obj->loc.type != LOC_TILE)
					{
						continue;
					}
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

			obj_t* player_obj = get_obj(g_player_oid);
			if (!g_game_over && (player_obj == NULL || player_obj->life <= 0))
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
			log_seperator();
		}

		SDL_SetRenderDrawColor(g_renderer,
			g_color_bg_shadow.r, g_color_bg_shadow.g, g_color_bg_shadow.b, 255);
		SDL_RenderClear(g_renderer);

		{
			obj_t* player_obj = get_obj(g_player_oid);
			if (player_obj != NULL)
			{
				camera_move_smoothly(&camera, loc_tc(player_obj->loc), 0.035f);
			}
		}

		draw_viewed_tiles(camera);

		/* Display some information in a corner. */
		{
			int y = 10;

			{
				obj_t* player_obj = get_obj(g_player_oid);
				if (player_obj != NULL)
				{
					char* text = format("HP: %d", get_obj(g_player_oid)->life);
					draw_text_sc(text,
						rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
					free(text);
					y += 30;
				}
			}

			if (g_game_over)
			{
				draw_text_sc(g_game_over_cause,
					rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
				y += 30;
			}
			
			{
				char* text = format("FPS: %d", fps_in_some_recent_iteration);
				draw_text_sc(text,
					rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
				free(text);
				y += 30;
			}

			#if 0
			{
				char* text = format("Time: %.1f s", (float)g_game_duration / 1000.0f);
				draw_text_sc(text,
					rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
				free(text);
				y += 30;
			}
			#endif

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

		iteration_number++;

		int end_iteration_time = SDL_GetTicks();
		iteration_duration = end_iteration_time - start_iteration_time;

		if (iteration_number % 20 == 0)
		{
			fps_in_some_recent_iteration =
				iteration_duration == 0 ? 0.0f : 1000 / iteration_duration;
		}
	}

	TTF_Quit();
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}
