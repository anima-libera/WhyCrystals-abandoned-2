
#ifndef WHYCRYSTALS_HEADER_OBJECTS_
#define WHYCRYSTALS_HEADER_OBJECTS_

#include "tc.h"
#include <stdbool.h>

/* A location that represents a place in the world. */
struct loc_t
{
	tc_t tc;
	/* NOTE: At some point a `loc_t` will be able to represent the fact of being
	 * contained by an other object instead of just being on a tile. */
};
typedef struct loc_t loc_t;

tc_t loc_tc(loc_t loc);
loc_t tc_to_loc(tc_t tc);

enum obj_type_t
{
	OBJ_NONE,
	OBJ_PLAYER,
	OBJ_CRYSTAL,
	OBJ_ROCK,
	OBJ_GRASS,
	OBJ_BUSH,
	OBJ_MOSS,
	OBJ_TREE,
};
typedef enum obj_type_t obj_type_t;

/* Object ID.
 * An `oid_t` refers to an object of the `g_obj_da` global dynamic array of objects.
 * References to objects should be `oid_t`s instead of pointers. That is because
 * when an object is destroyed, its `oid_t` is still safe to use (it just does not
 * point to an object anymore) unlike a pointer.
 * Note that it never makes sens to modify an `oid_t`. */
struct oid_t
{
	/* Index in `g_obj_da`. */
	int index;
	/* Generation that must match the generation of the `index`-th entry.
	 * If it does not match then it means the object referenced by this id
	 * has been destroyed. */
	int generation;
};
typedef struct oid_t oid_t;

bool oid_eq(oid_t oid_a, oid_t oid_b);

oid_t obj_alloc(obj_type_t type, loc_t loc);
void assert_oid_makes_sens(oid_t oid);
void obj_dealloc(oid_t oid);
typedef struct obj_t obj_t;
obj_t* get_obj(oid_t oid);

void obj_move(oid_t oid, tc_t dst_tc);

struct oid_da_t
{
	/* May contain null `oid_t`s. */
	oid_t* arr;
	int len, cap;
};
typedef struct oid_da_t oid_da_t;

void oid_da_add(oid_da_t* da, oid_t oid);
void oid_da_remove(oid_da_t* da, oid_t oid);

oid_t oid_da_find_type(oid_da_t const* da, obj_type_t type);
bool oid_da_contains_type(oid_da_t const* da, obj_type_t type);

extern oid_t g_crystal_oid;
extern oid_t g_player_oid;

enum visual_effect_type_t
{
	VISUAL_EFFECT_MOVE,
	VISUAL_EFFECT_DAMAGED,
};
typedef enum visual_effect_type_t visual_effect_type_t;

/* TODO: Make this better. */
struct visual_effect_t
{
	int t;
	int t_max;
	visual_effect_type_t type;
	tm_t src;
};
typedef struct visual_effect_t visual_effect_t;

struct obj_t
{
	obj_type_t type;
	loc_t loc;

	int life;

	visual_effect_t visual_effect;
};
typedef struct obj_t obj_t;

#endif /* WHYCRYSTALS_HEADER_OBJECTS_ */
