
#ifndef WHYCRYSTALS_HEADER_OBJECTS_
#define WHYCRYSTALS_HEADER_OBJECTS_

#include "tc.h"
#include <stdbool.h>

/* Object ID.
 * An `oid_t` refers to an object of the `g_obj_da` global dynamic array of objects.
 * References to objects should be `oid_t`s instead of pointers. That is because
 * when an object is destroyed, its `oid_t` is still safe to use (it just does not
 * point to an object anymore) unlike a pointer.
 * Note that it should not make sens to modify an `oid_t`
 * (except when itereating over all the objects). */
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

enum loc_type_t
{
	LOC_NONE,
	LOC_ON_TILE,
	LOC_IN_OBJ,
};
typedef enum loc_type_t loc_type_t;

/* A location that represents a place in the world. */
struct loc_t
{
	loc_type_t type;
	union
	{
		tc_t on_tile_tc;
		oid_t in_obj_oid;
	};
};
typedef struct loc_t loc_t;

tc_t loc_tc(loc_t loc);
loc_t tc_to_loc(tc_t tc);
loc_t in_obj_loc(oid_t container_oid);

enum obj_type_t
{
	OBJ_PLAYER,
	OBJ_CRYSTAL,
	OBJ_ROCK,
	OBJ_GRASS,
	OBJ_BUSH,
	OBJ_MOSS,
	OBJ_TREE,
	OBJ_SLIME,
	OBJ_CATERPILLAR,
};
typedef enum obj_type_t obj_type_t;

/* `0` cannot be the generation of an entry in `g_obj_da`, so `(oid_t){0}`
 * can be used to represent a null id or something. */
#define OID_NULL (oid_t){0}

bool oid_eq(oid_t oid_a, oid_t oid_b);

oid_t obj_create(obj_type_t type, loc_t loc);
void assert_oid_makes_sens(oid_t oid);
void obj_destroy(oid_t oid);
typedef struct obj_t obj_t;
obj_t* get_obj(oid_t oid);

void obj_move_tc(oid_t oid, tc_t dst_tc);

void obj_change_loc(oid_t oid, loc_t new_loc);

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
bool oid_da_contains_type_f(oid_da_t const* da, bool (*f)(obj_type_t type));

extern oid_t g_crystal_oid;
extern oid_t g_player_oid;

enum visual_effect_type_t
{
	VISUAL_EFFECT_NONE,
	VISUAL_EFFECT_MOVE,
	VISUAL_EFFECT_ATTACK,
	VISUAL_EFFECT_DAMAGED,
};
typedef enum visual_effect_type_t visual_effect_type_t;

/* TODO: Make this better or something ? */
struct visual_effect_t
{
	int t_begin;
	int t_end;
	visual_effect_type_t type;
	tm_t dir;
};
typedef struct visual_effect_t visual_effect_t;

struct visual_effect_da_t
{
	visual_effect_t* arr;
	int len, cap;
};
typedef struct visual_effect_da_t visual_effect_da_t;

void visual_effect_da_add(visual_effect_da_t* da, visual_effect_t visual_effect);
void visual_effect_da_remove(visual_effect_da_t* da, int index);

/* Object.
 * Pretty much everything that physically exists
 * in the world is or should be an object. */
struct obj_t
{
	obj_type_t type;
	loc_t loc;
	oid_da_t contained_da;
	int life;

	visual_effect_da_t visual_effect_da;
};
typedef struct obj_t obj_t;

/* Iterate over all the objects there is, like so:
 *    oid_t oid = OID_NULL;
 *    while (oid_iter(&oid)) {...}
 * Every oid will be refering to an existing object.
 * Note that it is even possible to destroy the referred object. */
bool oid_iter(oid_t* oid);

#endif /* WHYCRYSTALS_HEADER_OBJECTS_ */
