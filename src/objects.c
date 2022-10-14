
#include "objects.h"
#include "mapgrid.h"
#include "utils.h"
#include <limits.h>
#include <assert.h>

tc_t loc_tc(loc_t loc)
{
	switch (loc.type)
	{
		case LOC_ON_TILE:
			return loc.on_tile_tc;
		case LOC_IN_OBJ:
			assert(get_obj(loc.in_obj_oid) != NULL);
			return loc_tc(get_obj(loc.in_obj_oid)->loc);
		default:
			assert(false); exit(EXIT_FAILURE);
	}
}

loc_t tc_to_loc(tc_t tc)
{
	return (loc_t){.type = LOC_ON_TILE, .on_tile_tc = tc};
}

loc_t in_obj_loc(oid_t container_oid)
{
	assert(get_obj(container_oid) != NULL);
	return (loc_t){.type = LOC_IN_OBJ, .in_obj_oid = container_oid};
}

struct obj_entry_t
{
	bool exists;
	int generation;
	obj_t* obj;
};
typedef struct obj_entry_t obj_entry_t;

/* The global dynamic array of objects.
 * All objects should be stored in there. */
obj_entry_t* g_obj_da;
int g_obj_da_len, g_obj_da_cap;

bool oid_eq(oid_t oid_a, oid_t oid_b)
{
	return oid_a.index == oid_b.index && oid_a.generation == oid_b.generation;
}

static void obj_set_loc(oid_t oid, loc_t loc)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	switch (loc.type)
	{
		case LOC_ON_TILE:
			oid_da_add(&get_tile(loc_tc(loc))->oid_da, oid);
			obj->loc = loc;
		break;
		case LOC_IN_OBJ:
			oid_da_add(&get_obj(loc.in_obj_oid)->contained_da, oid);
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
	switch (obj->loc.type)
	{
		case LOC_ON_TILE:
			oid_da_remove(&get_tile(loc_tc(obj->loc))->oid_da, oid);
			obj->loc = (loc_t){.type = LOC_NONE};
		break;
		case LOC_IN_OBJ:
			oid_da_remove(&get_obj(obj->loc.in_obj_oid)->contained_da, oid);
			obj->loc = (loc_t){.type = LOC_NONE};
		break;
		default:
			assert(false); exit(EXIT_FAILURE);
	}
}

oid_t obj_create(obj_type_t type, loc_t loc)
{
	int index;
	for (int i = 0; i < g_obj_da_len; i++)
	{
		if (!g_obj_da[i].exists)
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
	*obj = (obj_t){.type = type, .loc = loc};
	entry->obj = obj;
	entry->exists = true;
	entry->generation++;

	oid_t oid = {.index = index, .generation = entry->generation};
	obj_set_loc(oid, loc);
	return oid;
}

/* Checks for `oid_t`s that were probably not returned by `obj_create` or corrupted.
 * Does not errors on `OID_NULL` (because these make sens). */
void assert_oid_makes_sens(oid_t oid)
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
	if (entry->generation == oid.generation)
	{
		/* If the object being destroyed contained subobjects,
		 * then now the subobjects have to be located at the same
		 * place as the container was. */
		for (int i = 0; i < entry->obj->contained_da.len; i++)
		{
			obj_change_loc(entry->obj->contained_da.arr[i], entry->obj->loc);
		}

		obj_unset_loc(oid);
		entry->exists = false;
	}
	else
	{
		/* The object was already destroyed.
		 * Should this be an error or something ? Or should destroying the same object
		 * multiple times just be ignored ? */
	}
}

/* Returns NULL if the oid does not refer to an existing object. */
obj_t* get_obj(oid_t oid)
{
	assert_oid_makes_sens(oid);
	obj_entry_t* entry = &g_obj_da[oid.index];
	if (entry->generation == oid.generation)
	{
		return entry->obj;
	}
	else
	{
		return NULL;
	}
}

void obj_move_tc(oid_t oid, tc_t dst_tc)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	assert(obj->loc.type == LOC_ON_TILE);
	obj_unset_loc(oid);
	obj_set_loc(oid, tc_to_loc(dst_tc));
}

void obj_change_loc(oid_t oid, loc_t new_loc)
{
	assert(get_obj(oid) != NULL);
	obj_unset_loc(oid);
	obj_set_loc(oid, new_loc);
}

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

bool oid_da_contains_type_f(oid_da_t const* da, bool (*f)(obj_type_t type))
{
	for (int i = 0; i < da->len; i++)
	{
		obj_t* obj = get_obj(da->arr[i]);
		if (obj != NULL && f(obj->type))
		{
			return true;
		}
	}
	return false;
}

oid_t g_crystal_oid = {0};
oid_t g_player_oid = {0};

void visual_effect_da_add(visual_effect_da_t* da, visual_effect_t visual_effect)
{
	DA_LENGTHEN(da->len += 1, da->cap, da->arr, visual_effect_t);
	da->arr[da->len-1] = visual_effect;
}

void visual_effect_da_remove(visual_effect_da_t* da, int index)
{
	assert(0 <= index && index < da->len);
	da->arr[index].type = VISUAL_EFFECT_NONE;

	/* If all the effects are "NONE" (i.e. nothing), then we can just free the list. */
	for (int i = 0; i < da->len; i++)
	{
		if (da->arr[i].type != VISUAL_EFFECT_NONE)
		{
			return;
		}
	}
	free(da->arr);
	*da = (visual_effect_da_t){0};
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
		if (g_obj_da[oid->index].exists)
		{
			oid->generation = g_obj_da[oid->index].generation;
			return true;
		}
		oid->index++;
	}
	return false;
}
