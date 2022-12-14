
#include "generators.h"
#include <assert.h>

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
		OBJ_LIQUID,
		OBJ_SLIME, OBJ_CATERPILLAR};
	obj_type_t obj_type = obj_types[rand() % (sizeof obj_types / sizeof obj_types[0])];

	material_type_t material_type = MATERIAL_HARD;
	switch (obj_type)
	{
		case OBJ_ROCK:
			material_type = MATERIAL_HARD;
		break;
		case OBJ_TREE:
		case OBJ_BUSH:
		case OBJ_GRASS:
		case OBJ_MOSS:
			material_type = MATERIAL_VEGETAL;
		break;
		case OBJ_SLIME:
		case OBJ_CATERPILLAR:
			material_type = MATERIAL_TISSUE;
		break;
		case OBJ_LIQUID:
			material_type = MATERIAL_LIQUID;
		break;
		default:
			assert(false); exit(EXIT_FAILURE);
		break;
	}

	material_id_t material_id = rand_material(material_type);
	material_id_t rare_material_id = rand_material(material_type);
	int rare_material_probability = rand() % (RARE_MATERIAL_PROBABILITY_MAX / 5);

	int life_min = 1 + rand() % 5;
	int life_max = life_min + rand() % 5;

	return (obj_gen_t){
		.obj_type = obj_type,
		.life_min = life_min,
		.life_max = life_max,
		.material_id = material_id,
		.rare_material_id = rare_material_id,
		.rare_material_probability = rare_material_probability,
	};
}

/* Section `biome_gen_t`. */

biome_gen_t biome_gen_generate(void)
{
	biome_gen_t biome_gen = {0};
	biome_gen.gen_number = 10;
	biome_gen.gen_probabilities =
		malloc(biome_gen.gen_number * sizeof biome_gen.gen_probabilities[0]);
	for (int i = 0; i < biome_gen.gen_number; i++)
	{
		int probability = rand() % 100 + 1;
		biome_gen.probability_sum += probability;
		biome_gen.gen_probabilities[i] = (gen_probabilities_t){
			.gen = obj_gen_generate(),
			.probability = probability};
	}
	return biome_gen;
}
