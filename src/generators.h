
#ifndef WHYCRYSTALS_HEADER_GENERATORS_
#define WHYCRYSTALS_HEADER_GENERATORS_

#include "objects.h"

/* Section `obj_gen_t`. */

/* Object generator.
 * It can generate objects of a specific kind. */
struct obj_gen_t
{
	obj_type_t obj_type;
	int life_min, life_max;
	material_id_t material_id;
	material_id_t rare_material_id;
	int rare_material_probability;
	#define RARE_MATERIAL_PROBABILITY_MAX 10000
};
typedef struct obj_gen_t obj_gen_t;

oid_t obj_generate(obj_gen_t* gen, loc_t loc);

obj_gen_t obj_gen_generate(void);

#endif /* WHYCRYSTALS_HEADER_GENERATORS_ */
