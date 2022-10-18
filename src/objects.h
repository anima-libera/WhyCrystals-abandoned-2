
#ifndef WHYCRYSTALS_HEADER_OBJECTS_
#define WHYCRYSTALS_HEADER_OBJECTS_

#include "tc.h"
#include "rendering.h"
#include "game.h"
#include <stdbool.h>

/* Section `oid_t`. */

/* An object ID in the global table of all objects.
 * An `oid_t` refers to an object of the `g_obj_da` global dynamic array of objects.
 * References to objects should be `oid_t`s instead of pointers. That is because
 * when an object is destroyed, its `oid_t` is still safe to use (it just does not
 * point to an object anymore) unlike a pointer.
 * Note that it should not make sens to modify an `oid_t` or to construct one manually
 * outside of `objects.c`. */
struct oid_t
{
	/* Index in `g_obj_da`. */
	int index;
	/* Generation that must match the generation of the `index`-th entry.
	 * If it does not match then it means the object referenced by this id
	 * has been destroyed and an newer object occupies this slot. */
	int generation;
};
typedef struct oid_t oid_t;

/* `0` cannot be the generation of any used entry in `g_obj_da` (minimum is 1),
 * so `(oid_t){0}` can be used to represent a null id or something. */
#define OID_NULL (oid_t){0}

bool oid_eq(oid_t oid_a, oid_t oid_b);

/* Section `loc_t`. */

/* Every existing object must have a `loc_t` (a location in the world), an object
 * cannot exist and be nowhere.
 * `oid_t` is defined before that because it is used in the definition of `loc_t`. */

enum loc_type_t
{
	/* An object must not be left without a location.
	 * Giving the none location to an object must be either temporary
	 * or the object is to be destroyed. */
	LOC_NONE,
	/* The object is on a tile. */
	LOC_TILE,
	/* The object is inside of an other object. */
	LOC_INSIDE_OBJ,
};
typedef enum loc_type_t loc_type_t;

/* A location that represents a place in the world. */
struct loc_t
{
	loc_type_t type;
	union
	{
		tc_t tile_tc;
		oid_t inside_obj_oid;
	};
};
typedef struct loc_t loc_t;

char* loc_to_text_allocated(loc_t loc);

tc_t loc_tc(loc_t loc);
loc_t tc_to_loc(tc_t tc);
loc_t inside_obj_loc(oid_t container_oid);

/* Section `visual_effect_obj_type_t`. */

/* TODO: Abreviate `visual_effect` by `ve` maybe. */

enum visual_effect_obj_type_t
{
	VISUAL_EFFECT_OBJ_NONE,
	VISUAL_EFFECT_OBJ_MOVE,
	VISUAL_EFFECT_OBJ_ATTACK,
	VISUAL_EFFECT_OBJ_DAMAGED,
};
typedef enum visual_effect_obj_type_t visual_effect_obj_type_t;

/* Visual effect for objects. */
struct visual_effect_obj_t
{
	int time_begin;
	int time_end;
	visual_effect_obj_type_t type;
	tm_t dir;
};
typedef struct visual_effect_obj_t visual_effect_obj_t;

struct visual_effect_obj_da_t
{
	visual_effect_obj_t* arr;
	int len, cap;
};
typedef struct visual_effect_obj_da_t visual_effect_obj_da_t;

void visual_effect_obj_da_add(visual_effect_obj_da_t* da, visual_effect_obj_t visual_effect_obj);
void visual_effect_obj_da_remove(visual_effect_obj_da_t* da, int index);

/* Section `oid_da_t`. */

struct oid_da_t
{
	/* May contain null `oid_t`s.
	 * Should not be treated as if it is in a particular order. */
	oid_t* arr;
	int len, cap;
};
typedef struct oid_da_t oid_da_t;

void oid_da_add(oid_da_t* da, oid_t oid);
void oid_da_remove(oid_da_t* da, oid_t oid);

/* Section `obj_t`. */

/* Pretty much everything that physically exist in the game is an object,
 * this allows for emergent gameplay (hopefully) and must remain a core principle
 * of the architecture of this game. */

enum obj_type_t
{
	OBJ_PLAYER,
	OBJ_CRYSTAL,
	OBJ_ROCK,
	OBJ_GRASS,
	OBJ_BUSH,
	OBJ_MOSS,
	OBJ_TREE,
	OBJ_SEED,
	OBJ_SLIME,
	OBJ_CATERPILLAR,
	OBJ_EGG,
	OBJ_WATER,

	OBJ_TYPE_NUMBER
};
typedef enum obj_type_t obj_type_t;

char const* obj_type_name(obj_type_t type);

/* An object of the game.
 * Pretty much everything that physically exists
 * in the world is or should be an object. */
struct obj_t
{
	obj_type_t type;
	loc_t loc;
	oid_da_t contained_da;
	int life;
	int max_life;
	material_id_t material_id;
	int age;

	visual_effect_obj_da_t visual_effect_da;
};
typedef struct obj_t obj_t;

oid_t obj_create(obj_type_t type, loc_t loc, int max_life, material_id_t material_id);
void obj_destroy(oid_t oid);
obj_t* get_obj(oid_t oid);

void obj_change_loc(oid_t oid, loc_t new_loc);

/* Iterate over all the objects there is, like so:
 *    oid_t oid = OID_NULL;
 *    while (oid_iter(&oid)) {...}
 * At every step the oid will be refering to an existing object.
 * Note that it is even possible to destroy the referred object. */
bool oid_iter(oid_t* oid);

oid_t rand_oid(void);

extern oid_t g_player_oid;

oid_t oid_da_find_type(oid_da_t const* da, obj_type_t type);
bool oid_da_contains_type(oid_da_t const* da, obj_type_t type);
bool oid_da_contains_obj_f(oid_da_t const* da, bool (*f)(oid_t oid));

/* Section dedicated to object properties, behaviors and related systems. */

char const* obj_name(oid_t oid);
bool obj_is_blocking(oid_t oid);
bool obj_can_get_hit_for_now(oid_t oid);
int obj_vision_blocking(oid_t oid);
rgb_t obj_foreground_color(oid_t oid);
char const* obj_text_representation(oid_t oid);
int obj_text_representation_stretch(oid_t oid);
rgb_t obj_background_color(oid_t oid);
bool obj_is_liquid(oid_t oid);
bool obj_moves_on_its_own(oid_t oid);

#endif /* WHYCRYSTALS_HEADER_OBJECTS_ */
