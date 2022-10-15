
#include "objects.h"
#include "mapgrid.h"
#include "utils.h"
#include <limits.h>
#include <assert.h>

/* Section `oid_t`. */

bool oid_eq(oid_t oid_a, oid_t oid_b)
{
	return oid_a.index == oid_b.index && oid_a.generation == oid_b.generation;
}

/* Section `loc_t`. */

tc_t loc_tc(loc_t loc)
{
	switch (loc.type)
	{
		case LOC_TILE:
			return loc.tile_tc;
		case LOC_INSIDE_OBJ:
			assert(get_obj(loc.inside_obj_oid) != NULL);
			return loc_tc(get_obj(loc.inside_obj_oid)->loc);
		default:
			assert(false); exit(EXIT_FAILURE);
	}
}

loc_t tc_to_loc(tc_t tc)
{
	return (loc_t){.type = LOC_TILE, .tile_tc = tc};
}

loc_t inside_obj_loc(oid_t container_oid)
{
	assert(get_obj(container_oid) != NULL);
	return (loc_t){.type = LOC_INSIDE_OBJ, .inside_obj_oid = container_oid};
}

/* Section `visual_effect_obj_type_t`. */

void visual_effect_obj_da_add(visual_effect_obj_da_t* da, visual_effect_obj_t visual_effect)
{
	DA_LENGTHEN(da->len += 1, da->cap, da->arr, visual_effect_obj_t);
	da->arr[da->len-1] = visual_effect;
}

void visual_effect_obj_da_remove(visual_effect_obj_da_t* da, int index)
{
	assert(0 <= index && index < da->len);
	da->arr[index].type = VISUAL_EFFECT_OBJ_NONE;

	/* If all the effects are "NONE" (i.e. nothing), then we can just free the list. */
	for (int i = 0; i < da->len; i++)
	{
		if (da->arr[i].type != VISUAL_EFFECT_OBJ_NONE)
		{
			return;
		}
	}
	free(da->arr);
	*da = (visual_effect_obj_da_t){0};
}

/* Section `oid_da_t`. */

void oid_da_add(oid_da_t* da, oid_t oid)
{
	/* Try inserting in a free spot. */
	for (int i = 0; i < da->len; i++)
	{
		if (oid_eq(da->arr[i], OID_NULL))
		{
			da->arr[i] = oid;
			return;
		}
	}
	/* No free spot, extend the dynamic array. */
	DA_LENGTHEN(da->len += 1, da->cap, da->arr, oid_t);
	da->arr[da->len-1] = oid;
}

void oid_da_remove(oid_da_t* da, oid_t oid)
{
	for (int i = 0; i < da->len; i++)
	{
		if (oid_eq(da->arr[i], oid))
		{
			da->arr[i] = OID_NULL;
		}
	}
}

/* Section `obj_t`. */

struct obj_entry_t
{
	bool used;
	int generation;
	obj_t* obj;
};
typedef struct obj_entry_t obj_entry_t;

/* The global dynamic array of objects.
 * All objects should be stored in there. */
static obj_entry_t* g_obj_da;
static int g_obj_da_len, g_obj_da_cap;

static void obj_set_loc(oid_t oid, loc_t loc)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	assert(obj->loc.type == LOC_NONE
		/* This function does not removes properly the object from its previous location
		 * as this is the job of `obj_unset_loc`. The object is thus expected to be nowhere,
		 * which is the case if it was just created or its location was just `obj_unset_loc`.*/);
	switch (loc.type)
	{
		case LOC_TILE:
			oid_da_add(&get_tile(loc_tc(loc))->oid_da, oid);
			obj->loc = loc;
		break;
		case LOC_INSIDE_OBJ:
			oid_da_add(&get_obj(loc.inside_obj_oid)->contained_da, oid);
			obj->loc = loc;
		break;
		default:
			assert(false); exit(EXIT_FAILURE);
	}
}

static void obj_unset_loc(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	assert(obj->loc.type != LOC_NONE
		/* It should not make sense to unset the location of on object
		 * that does not have a location ? I mean... It if ever happens
		 * then it is probably a bug. */);
	switch (obj->loc.type)
	{
		case LOC_TILE:
			oid_da_remove(&get_tile(loc_tc(obj->loc))->oid_da, oid);
			obj->loc = (loc_t){.type = LOC_NONE};
		break;
		case LOC_INSIDE_OBJ:
			oid_da_remove(&get_obj(obj->loc.inside_obj_oid)->contained_da, oid);
			obj->loc = (loc_t){.type = LOC_NONE};
		break;
		default:
			assert(false); exit(EXIT_FAILURE);
	}
}

oid_t obj_create(obj_type_t type, loc_t loc, int max_life)
{
	int index;
	for (int i = 0; i < g_obj_da_len; i++)
	{
		if (!g_obj_da[i].used)
		{
			index = i;
			goto index_found;
		}
	}
	assert(g_obj_da_len < INT_MAX);
	DA_LENGTHEN(g_obj_da_len += 1, g_obj_da_cap, g_obj_da, obj_entry_t);
	g_obj_da[g_obj_da_len-1] = (obj_entry_t){0};
	index = g_obj_da_len-1;

	index_found:;
	obj_entry_t* entry = &g_obj_da[index];
	obj_t* obj = malloc(sizeof(obj_t));
	*obj = (obj_t){
		.type = type,
		.loc = (loc_t){.type = LOC_NONE},
		.life = max_life,
		.max_life = max_life};
	entry->obj = obj;
	entry->used = true;
	entry->generation++;
	oid_t oid = {.index = index, .generation = entry->generation};

	obj_set_loc(oid, loc);
	return oid;
}

/* Checks for `oid_t`s that were probably not returned by `obj_create` or corrupted.
 * Does not errors on `OID_NULL` (because these make sens). */
static void assert_oid_makes_sens(oid_t oid)
{
	if (oid_eq(oid, OID_NULL))
	{
		return;
	}
	assert(0 <= oid.index && oid.index < g_obj_da_len);
	obj_entry_t* entry = &g_obj_da[oid.index];
	assert(oid.generation > 0);
	assert(entry->generation >= oid.generation
		/* If the oid of the object to destroy has a bigger generation than the
		 * entry of its index, then either the index is wrong or we somehow we ended up
		 * wanting to destroy an object that was not yet created.
		 * This happens either if there is a bug in the `g_obj_da` handling code or if
		 * the given oid is corrupted. */);
	#ifndef DEBUG
		(void)entry;
	#endif
}

void obj_destroy(oid_t oid)
{
	assert_oid_makes_sens(oid);
	assert(!oid_eq(oid, OID_NULL));
	obj_entry_t* entry = &g_obj_da[oid.index];
	if (entry->used && entry->generation == oid.generation)
	{
		/* If the object being destroyed contained subobjects,
		 * then now the subobjects have to be located at the same
		 * place as the container was. */
		for (int i = 0; i < entry->obj->contained_da.len; i++)
		{
			obj_change_loc(entry->obj->contained_da.arr[i], entry->obj->loc);
		}

		obj_unset_loc(oid);
		entry->used = false;
	}
	else
	{
		/* The object was already destroyed.
		 * Should this be an error or something ? Or should destroying the same object
		 * multiple times just be ignored ? Or log a warning somewhere ?
		 * TODO: Do something maybe. */
	}
}

/* Returns NULL if the oid does not refer to an existing object. */
obj_t* get_obj(oid_t oid)
{
	assert_oid_makes_sens(oid);
	if (oid_eq(oid, OID_NULL))
	{
		return NULL;
	}
	obj_entry_t* entry = &g_obj_da[oid.index];
	if (entry->used && entry->generation == oid.generation)
	{
		return entry->obj;
	}
	else
	{
		return NULL;
	}
}

void obj_change_loc(oid_t oid, loc_t new_loc)
{
	assert(get_obj(oid) != NULL);
	obj_unset_loc(oid);
	obj_set_loc(oid, new_loc);
}

bool oid_iter(oid_t* oid)
{
	assert(oid != NULL);
	assert_oid_makes_sens(*oid);
	if (!oid_eq(*oid, OID_NULL))
	{
		oid->index++;
	}
	while (oid->index < g_obj_da_len)
	{
		if (g_obj_da[oid->index].used)
		{
			oid->generation = g_obj_da[oid->index].generation;
			return true;
		}
		oid->index++;
	}
	return false;
}

oid_t rand_oid(void)
{
	int index = rand() % g_obj_da_len;
	while (!g_obj_da[index].used)
	{
		index = rand() % g_obj_da_len;
	}
	return (oid_t){.index = index, .generation = g_obj_da[index].generation};
}

oid_t g_player_oid = {0};

oid_t oid_da_find_type(oid_da_t const* da, obj_type_t type)
{
	for (int i = 0; i < da->len; i++)
	{
		obj_t* obj = get_obj(da->arr[i]);
		if (obj != NULL && obj->type == type)
		{
			return da->arr[i];
		}
	}
	return OID_NULL;
}

bool oid_da_contains_type(oid_da_t const* da, obj_type_t type)
{
	return !oid_eq(oid_da_find_type(da, type), OID_NULL);
}

bool oid_da_contains_obj_f(oid_da_t const* da, bool (*f)(oid_t oid))
{
	for (int i = 0; i < da->len; i++)
	{
		oid_t oid = da->arr[i];
		if (get_obj(oid) != NULL && f(oid))
		{
			return true;
		}
	}
	return false;
}

/* Section dedicated to object properties, behaviors and related systems. */

char const* obj_name(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
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
		case OBJ_WATER:       return "water";
		default:              return "thing";
	}
}

bool obj_is_blocking(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
	{
		case OBJ_TREE:
		case OBJ_ROCK:
		case OBJ_CRYSTAL:
		case OBJ_BUSH:
		case OBJ_SLIME:
		case OBJ_CATERPILLAR:
		case OBJ_PLAYER:
			return true;
		default:
			return false;
	}
}

bool obj_can_get_hit_for_now(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
	{
		case OBJ_BUSH:
		case OBJ_SLIME:
		case OBJ_CATERPILLAR:
		case OBJ_PLAYER:
			return true;
		default:
			return false;
	}
}

int obj_vision_blocking(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
	{
		case OBJ_GRASS:       return 3;
		case OBJ_BUSH:        return 4;
		case OBJ_MOSS:        return 1;
		case OBJ_TREE:        return 8;
		case OBJ_ROCK:        return 100;
		case OBJ_CRYSTAL:     return 4;
		case OBJ_SLIME:       return 2;
		case OBJ_CATERPILLAR: return 1;
		case OBJ_WATER:       return 1;
		default:              return 1;
	}
}

rgb_t obj_foreground_color(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
	{
		case OBJ_PLAYER:
			return g_color_yellow;
		case OBJ_CRYSTAL:
			return g_color_red;
		case OBJ_ROCK:
			return g_color_white;
		case OBJ_TREE:
			return g_color_light_green;
		case OBJ_BUSH:
			return g_color_green;
		case OBJ_SLIME:
			if (obj->contained_da.len > 9)
			{
				return g_color_red;
			}
			else if (obj->contained_da.len > 0)
			{
				return g_color_yellow;
			}
			else
			{
				return g_color_cyan;
			}
		case OBJ_CATERPILLAR:
			return g_color_yellow;
		case OBJ_GRASS:
			return g_color_dark_green;
		case OBJ_MOSS:
			return g_color_dark_green;
		case OBJ_WATER:
			return g_color_cyan;
		default:
			return g_color_white;
	}
}

char const* obj_text_representation(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
	{
		case OBJ_PLAYER:      return "@";
		case OBJ_CRYSTAL:     return "A";
		case OBJ_ROCK:        return "#";
		case OBJ_TREE:        return "Y";
		case OBJ_BUSH:        return "n";
		case OBJ_SLIME:       return "o";
		case OBJ_CATERPILLAR: return "~";
		case OBJ_GRASS:       return " v ";
		case OBJ_MOSS:        return " .. ";
		case OBJ_WATER:       return "~";
		default:              return "X";
	}
}

int obj_text_representation_stretch(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
	{
		case OBJ_BUSH: return 10;
		default:       return 0;
	}
}

rgb_t obj_background_color(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
	{
		case OBJ_WATER: return g_color_blue;
		default:        return g_color_bg;
	}
}

bool obj_is_liquid(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (obj->type)
	{
		case OBJ_WATER:
			return true;
		default:
			return false;
	}
}
