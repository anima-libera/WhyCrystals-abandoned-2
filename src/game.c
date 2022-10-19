
#include "game.h"
#include "utils.h"
#include "log.h"
#include <stdlib.h>
#include <assert.h>

int g_turn_number = 0;

/* Section `material_t`. */

char const* material_type_name(material_type_t type)
{
	switch (type)
	{
		case MATERIAL_HARD:    return "hard";
		case MATERIAL_VEGETAL: return "vegetal";
		case MATERIAL_TISSUE:  return "tissue";
		case MATERIAL_LIQUID:  return "liquid";
		default:               assert(false); exit(EXIT_FAILURE);
	}
}

char* generate_name(void)
{
	int pair_number = 1 + (rand() % 2) + (rand() % 20 == 0 ? 1 : 0);
	char* name = malloc(2 * pair_number + 1);
	name[2 * pair_number] = '\0';
	for (int i = 0; i < pair_number; i ++)
	{
		name[2 * i + 0] = "zrtpqsdfghjklmwxcvbn"[rand() % 20];
		name[2 * i + 1] = "aeyuio"[rand() % 6];
	}
	return name;
}

material_t* g_material_da;
int g_material_da_len, g_material_da_cap;

material_id_t generate_material(material_type_t type)
{
	DA_LENGTHEN(g_material_da_len += 1,
		g_material_da_cap, g_material_da, material_t);
	material_id_t id = g_material_da_len-1;

	char* name = rand() % 2 == 0 ?
		format("%s %d", material_type_name(type), id) :
		format("%s %s", material_type_name(type), generate_name());

	rgb_t colors[2] = {
		{rand() % 255, rand() % 255, rand() % 255},
		{rand() % 255, rand() % 255, rand() % 255}};
	for (int i = 0; i < 2; i++)
	{
		switch (type)
		{
			case MATERIAL_HARD:
				((uint8_t*)&colors[i])[rand() % 3] = (255 - 40) + rand() % 40;;
			break;
			case MATERIAL_VEGETAL:
				colors[i].r = rand() % 120;
				colors[i].g = (255 - 40) + rand() % 40;
				colors[i].b = rand() % 120;
			break;
			case MATERIAL_TISSUE:
				colors[i].r = (255 - 100) + rand() % 100;
				colors[i].g = (255 - 100) + rand() % 100;
				colors[i].b = (255 - 100) + rand() % 100;
				((uint8_t*)&colors[i])[rand() % 3] = 0;
			break;
			case MATERIAL_LIQUID:
				colors[i].r = (255 - 80) + rand() % 80;
				colors[i].g = (255 - 80) + rand() % 80;
				colors[i].b = (255 - 80) + rand() % 80;
				((uint8_t*)&colors[i])[rand() % 3] = rand() % 80;
			break;
		}
	}

	g_material_da[id] = (material_t){
		.name = name,
		.type = type,
		.primary_color = colors[0],
		.secondary_color = colors[1]};
	return id;
}

void generate_some_materials(void)
{
	for (int i = 0; i < 3; i++)
	{
		generate_material(MATERIAL_HARD);
	}
	for (int i = 0; i < 3; i++)
	{
		generate_material(MATERIAL_VEGETAL);
	}
	for (int i = 0; i < 3; i++)
	{
		generate_material(MATERIAL_TISSUE);
	}
	for (int i = 0; i < 3; i++)
	{
		generate_material(MATERIAL_LIQUID);
	}

	printf("Generated %d materials: ", g_material_da_len);
	for (int i = 0; i < g_material_da_len; i++)
	{
		printf("%s%s", g_material_da[i].name, i == g_material_da_len-1 ? "\n" : ", ");
	}
}

material_t* get_material(material_id_t id)
{
	assert(0 <= id && id < g_material_da_len);
	return &g_material_da[id];
}

material_id_t rand_material(material_type_t type)
{
	assert(g_material_da_len > 0);
	material_id_t material_id = rand() % g_material_da_len;
	while (get_material(material_id)->type != type)
	{
		material_id = rand() % g_material_da_len;
	}
	return material_id;
}
