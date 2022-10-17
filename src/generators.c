
#include "generators.h"

/* Section `obj_gen_t`. */

oid_t obj_generate(obj_gen_t* gen, loc_t loc)
{
	int life = gen->life_min + rand() % (gen->life_max - gen->life_min + 1);
	material_id_t material_id =
		rand() < RARE_MATERIAL_PROBABILITY_MAX ? gen->rare_material_id : gen->material_id;
	oid_t oid = obj_create(gen->obj_type, loc, life, material_id);
	return oid;
}

obj_gen_t obj_gen_generate(void)
{
	obj_type_t obj_types[] = {
		OBJ_TREE, OBJ_BUSH, OBJ_ROCK, OBJ_GRASS, OBJ_MOSS,
		OBJ_WATER,
		OBJ_SLIME, OBJ_CATERPILLAR};
	obj_type_t obj_type = obj_types[rand() % (sizeof obj_types / sizeof obj_types[0])];

	int life_min = 1 + rand() % 5;
	int life_max = life_min + rand() % 5;

	material_id_t material_id = rand_material();
	material_id_t rare_material_id = rand_material();
	int rare_material_probability = rand() % (RARE_MATERIAL_PROBABILITY_MAX / 5);

	return (obj_gen_t){
		.obj_type = obj_type,
		.life_min = life_min,
		.life_max = life_max,
		.material_id = material_id,
		.rare_material_id = rare_material_id,
		.rare_material_probability = rare_material_probability,
	};
}
