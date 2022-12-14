
#include "embedded.h"
#include "utils.h"
#include "objects.h"
#include "mapgrid.h"
#include "rendering.h"
#include "log.h"
#include "materials.h"
#include "generators.h"
#include "tc.h"
#include "laws.h"
#include "gameloop.h"
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <stdarg.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

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

	tc_t src_tc = loc_to_tc(player_obj->loc);

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

struct text_particle_t
{
	int time_begin;
	int time_end;
	tcf_t tcf_begin;
	tcf_t tcf_end;
	char* text;
	rgba_t color;
};
typedef struct text_particle_t text_particle_t;

text_particle_t* text_particle_da;
int text_particle_da_len, text_particle_da_cap;

void create_text_particle(char* text, rgba_t color, tcf_t tcf, int duration)
{
	DA_LENGTHEN(text_particle_da_len += 1,
		text_particle_da_cap, text_particle_da, text_particle_t);
	text_particle_da[text_particle_da_len-1] = (text_particle_t){
		.time_begin = g_game_time,
		.time_end = g_game_time + duration,
		.tcf_begin = tcf,
		.tcf_end = {tcf.x, tcf.y - 1},
		.text = text,
		.color = color};
}

void draw_text_particles(camera_t camera)
{
	bool there_is_no_more_text_particles = true;
	for (int i = 0; i < text_particle_da_len; i++)
	{
		text_particle_t* text_particle = &text_particle_da[i];
		if (text_particle->time_end < g_game_time)
		{
			if (text_particle_da[i].text != NULL)
			{
				free(text_particle_da[i].text);
				text_particle_da[i] = (text_particle_t){0};
			}
			continue;
		}
		there_is_no_more_text_particles = false;

		/* Actually draw it. */
		int t = g_game_time - text_particle->time_begin;
		int t_max = text_particle->time_end - text_particle->time_begin;
		sc_t sc_begin = camera_tcf(camera, text_particle->tcf_begin);
		sc_t sc_end = camera_tcf(camera, text_particle->tcf_end);
		sc_t sc = {
			.x = interpolate(t, t_max, sc_begin.x, sc_end.x),
			.y = interpolate(t, t_max, sc_begin.y, sc_end.y)};
		draw_text_sc_center(text_particle->text, text_particle->color, FONT_TL, sc);
	}

	if (there_is_no_more_text_particles)
	{
		free(text_particle_da);
		text_particle_da = NULL;
		text_particle_da_len = 0;
		text_particle_da_cap = 0;
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
	tc_t tc_attacker = loc_to_tc(obj_attacker->loc);
	tc_t tc_target = loc_to_tc(obj_target->loc);
	tm_t dir = tc_diff_as_tm(tc_attacker, tc_target);
	bool event_visible = get_tile(tc_attacker)->vision > 0 || get_tile(tc_target)->vision > 0;
	
	int damages = 1;
	if (event_visible)
	{
		create_text_particle(format("%d", damages),
			(rgba_t){255, 0, 0, 255},
			(tcf_t){(float)tc_target.x, (float)tc_target.y},
			400);
	}
	obj_target->life -= damages;
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
			.time_begin = g_game_time,
			.time_end = g_game_time + 100,
			.dir = dir});
		if (event_visible)
		{
			log_text("A %s hit a %s.",
				obj_name(oid_attacker), obj_name(oid_target));
		}
	}
	visual_effect_obj_da_add(&obj_attacker->visual_effect_da, (visual_effect_obj_t){
		.type = VISUAL_EFFECT_OBJ_ATTACK,
		.time_begin = g_game_time,
		.time_end = g_game_time + 80,
		.dir = dir});
}

void obj_try_move(oid_t oid, tm_t move)
{
	assert(get_obj(oid) != NULL);
	tc_t dst_tc = tc_add_tm(loc_to_tc(get_obj(oid)->loc), move);
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
		.time_begin = g_game_time,
		.time_end = g_game_time + 60,
		.dir = tm_reverse(move)});
}

void generate_map_path(void)
{
	tc_t crystal_tc = {
		.x = g_mg_rect.x + g_mg_rect.w / 4 + rand() % (g_mg_rect.w / 9),
		.y = g_mg_rect.y + g_mg_rect.h / 3 + rand() % (g_mg_rect.h / 3)};
	obj_create(OBJ_CRYSTAL, tc_to_loc(crystal_tc), 100, rand_material(MATERIAL_HARD));

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

			int keep_direction_force =
				same_direction_steps == 1 ? 6 :
				direction.x != 0 ? 4 :
				2;
			if (same_direction_steps >= 1 && rand() % keep_direction_force == 0)
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
}

void generate_map(void)
{
	generate_map_path();

	biome_gen_t biome_gens[9];
	for (int i = 0; i < (int)(sizeof biome_gens / sizeof biome_gens[0]); i++)
	{
		biome_gens[i] = biome_gen_generate();
	}

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = get_tile(tc);

		if (tile->is_path)
		{
			continue;
		}

		#if 0
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
		#endif

		biome_gen_t* biome_gen = &biome_gens[
			((tc.y * 3) / g_mg_rect.h) * 3 + (tc.x * 3) / g_mg_rect.w];

		obj_gen_t* gen = NULL;
		int r = rand() % biome_gen->probability_sum;
		for (int i = 0; i < biome_gen->gen_number; i++)
		{
			r -= biome_gen->gen_probabilities[i].probability;
			if (r < 0)
			{
				gen = &biome_gen->gen_probabilities[i].gen;
				break;
			}
		}

		oid_t oid = obj_generate(gen, tc_to_loc(tc));

		if (obj_moves_on_its_own(oid))
		{
			obj_create(OBJ_MOSS, tc_to_loc(tc), 1, rand_material(MATERIAL_VEGETAL));
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
			OBJ_EGG,
			OBJ_LIQUID,
			OBJ_GRASS,
			OBJ_SEED,
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
				if (g_game_time > ve->time_end)
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
					int t = g_game_time - ve->time_begin;
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

void perform_turn(void)
{
	apply_laws();

	if (g_game_has_started)
	{
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
		log_turn_seperator();
	}
}

void draw_obj_da(oid_da_t* oid_da)
{
	#warning TODO better GUI stuff

	int y = g_window_h - 10 - 30;
	for (int i = 0; i < oid_da->len; i++)
	{
		oid_t oid = oid_da->arr[i];
		if (oid_eq(oid, OID_NULL))
		{
			continue;
		}
		/* Draw a short text description of the object. */
		draw_text_sc(obj_name(oid),
			rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){g_window_w - 200, y});
		/* Draw the object visual representation. */
		SDL_Rect rect = {g_window_w - 200 - 10 - 25, y, 25, 25};
		char const* text = obj_text_representation(oid);
		int text_stretch = obj_text_representation_stretch(oid);
		rgb_t text_color = obj_foreground_color(oid);
		rect.y -= 5 + text_stretch;
		rect.h += 10 + text_stretch + text_stretch / 3;
		draw_text_rect(text, rgb_to_rgba(text_color, 255), FONT_RG, rect);
	
		y -= 30;
	}
}

void draw_objects_on_same_tile_list(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	assert(obj->loc.type = LOC_TILE);
	oid_da_t* tile_oid_da = &get_tile(loc_to_tc(obj->loc))->oid_da;
	draw_obj_da(tile_oid_da);
}

void init_all(void)
{
	printf("Initialize stuff\n");

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
	g_font_table[FONT_RG] = TTF_OpenFontRW(rwops_font, 0, 20);
	assert(g_font_table[FONT_RG] != NULL);

	rwops_font = SDL_RWFromConstMem(
		g_asset_font_2,
		g_asset_font_2_size);
	assert(rwops_font != NULL);
	g_font_table[FONT_TL] = TTF_OpenFontRW(rwops_font, 0, 30);
	assert(g_font_table[FONT_TL] != NULL);

	rwops_font = SDL_RWFromConstMem(
		g_asset_font_2,
		g_asset_font_2_size);
	assert(rwops_font != NULL);
	g_font_table[FONT_TL_SMALL] = TTF_OpenFontRW(rwops_font, 0, 15);
	assert(g_font_table[FONT_TL_SMALL] != NULL);

	g_mg_rect.w = 100;
	g_mg_rect.h = 100;
	g_mg = malloc(g_mg_rect.w * g_mg_rect.h * sizeof(tile_t));

	for (int y = 0; y < g_mg_rect.h; y++)
	for (int x = 0; x < g_mg_rect.w; x++)
	{
		tc_t tc = {x, y};
		tile_t* tile = get_tile(tc);
		*tile = (tile_t){0};
	}

	printf("Generate materials\n");
	generate_some_materials();
	printf("Generate map\n");
	generate_map();

	register_laws();

	printf("Perform some turns\n");
	for (int i = 0; i < 200; i++)
	{
		perform_turn();

		if (i % 50 == 0 && i > 0)
		{
			printf("Performing turns %d%%\n", (i * 100) / 200);
		}
	}
	printf("Performing turns done\n");

	printf("Spawn player\n");
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
		g_player_oid = obj_create(OBJ_PLAYER, tc_to_loc(tc),
			10, rand_material(MATERIAL_TISSUE));

		#warning TEST
		obj_create(OBJ_MOSS, inside_obj_loc(g_player_oid),
			10, rand_material(MATERIAL_VEGETAL));
		oid_t moss_oid = obj_create(OBJ_MOSS, inside_obj_loc(g_player_oid),
			10, rand_material(MATERIAL_VEGETAL));
		obj_create(OBJ_GRASS, inside_obj_loc(moss_oid),
			10, rand_material(MATERIAL_VEGETAL));
	}
	recompute_vision();
	
	camera_set(&g_camera, loc_to_tc(get_obj(g_player_oid)->loc));

	printf("Object count at the end of initialization: %d\n", g_obj_count);
}

void cleanup_all(void)
{
	printf("Cleanup stuff\n");
	TTF_Quit();
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
}

void draw_object_list_entry(oid_t oid, int y, int indent_level)
{
	/* Draw indentations. */
	SDL_SetRenderDrawColor(g_renderer, g_color_white.r, g_color_white.g, g_color_white.b, 255);
	for (int i = 0; i < indent_level; i++)
	{
		SDL_RenderDrawLine(g_renderer, 300 + i * 10, y, 300 + i * 10, y + 30);
	}
	
	/* Draw the object visual representation. */
	SDL_Rect rect = {300 + indent_level * 10, y, 25, 25};
	char const* text = obj_text_representation(oid);
	int text_stretch = obj_text_representation_stretch(oid);
	rgb_t text_color = obj_foreground_color(oid);
	rgb_t bg_color = obj_background_color(oid);
	SDL_SetRenderDrawColor(g_renderer, bg_color.r, bg_color.g, bg_color.b, 255);
	SDL_RenderFillRect(g_renderer, &rect);
	rect.y -= 5 + text_stretch;
	rect.h += 10 + text_stretch + text_stretch / 3;
	draw_text_rect(text, rgb_to_rgba(text_color, 255), FONT_RG, rect);

	/* Draw a short text description of the object. */
	draw_text_sc(obj_name(oid),
		rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){300 + indent_level * 10 + 30, y});
}

int draw_object_list_recursively(oid_t oid, int y, int indent_level)
{
	obj_t* obj = get_obj(oid);
	if (obj == NULL)
	{
		return y;
	}

	draw_object_list_entry(oid, y, indent_level);
	y += 30;

	for (int i = 0; i < obj->attached_da.len; i++)
	{
		oid_t sub_oid = obj->attached_da.arr[i];
		if (oid_eq(sub_oid, OID_NULL))
		{
			continue;
		}
		y = draw_object_list_recursively(sub_oid, y, indent_level + 1);
	}

	return y;
}

void internals_menu_game_state_draw_layer(void)
{
	draw_object_list_recursively(g_player_oid, 20, 0);
}

void internals_menu_game_handle_input_event_direction(input_event_direction_t input_event_direction)
{
	;
}

void internals_menu_game_handle_input_event_letter_key(char letter)
{
	;
}

void internals_menu_game_handle_input_event_debugging_letter_key(char letter)
{
	;
}

game_state_t internals_menu_game_state = {
	.draw_layer =
		internals_menu_game_state_draw_layer,
	.handle_input_event_direction =
		internals_menu_game_handle_input_event_direction,
	.handle_input_event_letter_key =
		internals_menu_game_handle_input_event_letter_key,
	.handle_input_event_debugging_letter_key =
		internals_menu_game_handle_input_event_debugging_letter_key};

void base_game_state_draw_layer(void)
{
	{
		obj_t* player_obj = get_obj(g_player_oid);
		if (player_obj != NULL)
		{
			camera_move_smoothly(&g_camera, loc_to_tc(player_obj->loc), 0.035f);
		}
	}

	draw_viewed_tiles(g_camera);
	draw_text_particles(g_camera);

	if (get_obj(g_player_oid) != NULL)
	{
		draw_objects_on_same_tile_list(g_player_oid);
	}

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
			char* text = format("FPS: %d", g_fps_in_some_recent_iteration);
			draw_text_sc(text,
				rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
			free(text);
			y += 30;
		}

		{
			char* text = format("Obj count: %d", g_obj_count);
			draw_text_sc(text,
				rgb_to_rgba(g_color_white, 255), FONT_RG, (sc_t){10, y});
			free(text);
			y += 30;
		}
	}

	draw_log();
}

void base_game_handle_input_event_direction(input_event_direction_t input_event_direction)
{
	if (!g_game_over)
	{
		tm_t dir = tm_from_input_event_direction(input_event_direction);
		obj_try_move(g_player_oid, dir);
		perform_turn();
	}
}

void base_game_handle_input_event_letter_key(char letter)
{
	switch (letter)
	{
		case 'i':
			push_game_state(internals_menu_game_state);
		break;
	}
}

void base_game_handle_input_event_debugging_letter_key(char letter)
{
	switch (letter)
	{
		case 'a':
		case 'q':
			/* Zoom in or out. */
			g_tile_w += 5 * (letter == 'a' ? 1 : -1);
			g_tile_h += 5 * (letter == 'a' ? 1 : -1);
		break;
		case 's':
			/* Commit sucide. */
			obj_destroy(g_player_oid);
			log_text("Game over.");
			g_game_over = true;
			g_game_over_cause = format("Killed by suicide xd.");
		break;
		case 'p':
			/* Possess a random object, abandonning the current body. */
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
					log_text("Possessing something somewhere.");
					perform_turn();
					break;
				}
			}
		break;
		case 'o':
			/* Produce some moss. */
			{
				obj_t* player_obj = get_obj(g_player_oid);
				if (player_obj != NULL)
				{
					obj_create(OBJ_MOSS, player_obj->loc,
						1, rand_material(MATERIAL_VEGETAL));
					log_text("Created moss.");
					perform_turn();
				}
			}
		break;
	}
}

game_state_t base_game_state = {
	.draw_layer =
		base_game_state_draw_layer,
	.handle_input_event_direction =
		base_game_handle_input_event_direction,
	.handle_input_event_letter_key =
		base_game_handle_input_event_letter_key,
	.handle_input_event_debugging_letter_key =
		base_game_handle_input_event_debugging_letter_key};

int main(void)
{
	main_start:

	init_all();
	push_game_state(base_game_state);
	enter_gameloop();
	cleanup_all();
	
	if (g_should_restart)
	{
		g_should_restart = false;
		goto main_start;
	}

	printf("Terminate execution normally\n");
	return 0;
}
