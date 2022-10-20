
#include "laws.h"
#include "utils.h"
#include "mapgrid.h"
#include <stdio.h>
#include <assert.h>

void law_crystal_healing_effect(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	if (obj->type == OBJ_CRYSTAL)
	{
		for (int i = 0; i < 4; i++)
		{
			tm_t tm = TM_ONE_ALL[i];
			tc_t neighbor_tc = tc_add_tm(loc_to_tc(obj->loc), tm);
			tile_t* neighbor_tile = get_tile(neighbor_tc);
			if (neighbor_tile == NULL)
			{
				continue;
			}
			for (int i = 0; i < neighbor_tile->oid_da.len; i++)
			{
				obj_t* obj = get_obj(neighbor_tile->oid_da.arr[i]);
				if (obj != NULL)
				{
					if (obj->life < obj->max_life)
					{
						obj->life++;
					}
				}
			}
		}
	}
}

void law_old_age_effect(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	if (obj->type == OBJ_SLIME || obj->type == OBJ_CATERPILLAR)
	{
		if (obj->age > 100 && rand() % 5 == 0)
		{
			obj->life--;
		}
	}
}

void law_slime_action(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	if (obj->type == OBJ_SLIME)
	{
		if (obj->age > 0 && obj->age % 50 == 0)
		{
			oid_t oid_egg = obj_create(OBJ_EGG, obj->loc, 1, rand_material(MATERIAL_HARD));
			obj_create(OBJ_SLIME, inside_obj_loc(oid_egg), obj->max_life, obj->max_life);
		}
		else
		{
			if (obj->loc.type == LOC_TILE && rand() % 3 == 0)
			{
				obj_try_move(oid, rand_tm_one());
			}
		}
	}
}

void law_egg_hatching(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	if (obj->type == OBJ_EGG)
	{
		if (obj->age >= 45 && rand() % 10 == 0)
		{
			obj_destroy(oid);
		}
	}
}

void law_caterpillar_action(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	if (obj->type == OBJ_CATERPILLAR)
	{
		if (obj->age > 0 && obj->age % 50 == 0)
		{
			obj_create(OBJ_CATERPILLAR, obj->loc, obj->max_life, obj->material_id);
		}
		else
		{
			if (obj->loc.type == LOC_TILE)
			{
				for (int i = 0; i < 4; i++)
				{
					tm_t tm = TM_ONE_ALL[i];
					tc_t dst_tc = tc_add_tm(loc_to_tc(obj->loc), tm);
					tile_t* dst_tile = get_tile(dst_tc);
					if (dst_tile == NULL)
					{
						continue;
					}
					if (oid_da_contains_type(&dst_tile->oid_da, OBJ_PLAYER))
					{
						obj_try_move(oid, tm);
						goto caterpillar_done_moving;
					}
				}
				obj_try_move(oid, rand_tm_one());
				caterpillar_done_moving:;
			}
		}
	}
}

void law_tree_action(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	if (obj->type == OBJ_TREE)
	{
		if (obj->loc.type == LOC_TILE && obj->age >= 45 && rand() % 40 == 0)
		{
			tc_t seed_tc = tc_add_tm(loc_to_tc(obj->loc), rand_tm_one());
			tile_t* seed_tile = get_tile(seed_tc);
			if (seed_tile != NULL)
			{
				bool seed_tile_blocked =
					oid_da_contains_obj_f(&get_tile(seed_tc)->oid_da, obj_is_blocking);
				if (!seed_tile_blocked)
				{
					obj_create(OBJ_SEED, tc_to_loc(seed_tc),
						1, rand_material(MATERIAL_VEGETAL));
				}
			}
		}
	}
}

void law_seed_growing(oid_t oid)
{
	obj_t* obj = get_obj(oid);
	assert(obj != NULL);
	if (obj->type == OBJ_SEED)
	{
		if (obj->loc.type == LOC_TILE && obj->age >= 45 && rand() % 10 == 0)
		{
			bool tile_blocked =
				oid_da_contains_obj_f(&get_tile(loc_to_tc(obj->loc))->oid_da, obj_is_blocking);
			if (!tile_blocked)
			{
				obj_create(OBJ_TREE, obj->loc, 7, rand_material(MATERIAL_VEGETAL));
				obj_destroy(oid);
			}
		}
	}
}

law_t* g_law_da = NULL;
int g_law_da_len = 0, g_law_da_cap = 0;

void register_law(law_t law)
{
	DA_LENGTHEN(g_law_da_len += 1, g_law_da_cap, g_law_da, law_t);
	g_law_da[g_law_da_len-1] = law;
	printf("Registered law %s\n", law.name);
}

void register_laws(void)
{
	#define REGISTER_LAW_FUNCTION(function_name_) \
		register_law((law_t){ \
			.name = #function_name_, \
			.function = function_name_})
	REGISTER_LAW_FUNCTION(law_crystal_healing_effect);
	REGISTER_LAW_FUNCTION(law_old_age_effect);
	REGISTER_LAW_FUNCTION(law_slime_action);
	REGISTER_LAW_FUNCTION(law_egg_hatching);
	REGISTER_LAW_FUNCTION(law_caterpillar_action);
	REGISTER_LAW_FUNCTION(law_tree_action);
	REGISTER_LAW_FUNCTION(law_seed_growing);
	#undef REGISTER_LAW_FUNCTION
}

void apply_laws(void)
{
	oid_t oid = OID_NULL;
	while (oid_iter(&oid))
	{
		obj_t* obj = get_obj(oid);
		obj->age++;

		for (int i = 0; i < g_law_da_len; i++)
		{
			g_law_da[i].function(oid);

			/* The object might have been destroyed. */
			obj = get_obj(oid);
			if (obj == NULL)
			{
				goto next_obj;
			}
		}

		/* The object might have been destroyed. */
		obj = get_obj(oid);
		if (obj == NULL)
		{
			continue;
		}

		if (obj->life <= 0)
		{
			obj_destroy(oid);
			continue;
		}

		next_obj:;
	}
}

