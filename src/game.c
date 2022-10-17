
#include "game.h"
#include "utils.h"
#include "log.h"
#include <stdlib.h>

int g_turn_number = 0;

/* Section `material_t`. */

material_t* g_material_da;
int g_material_da_len, g_material_da_cap;

material_id_t generate_one_material(void)
{
	DA_LENGTHEN(g_material_da_len += 1,
		g_material_da_cap, g_material_da, material_t);
	material_id_t id = g_material_da_len-1;
	g_material_da[id] = (material_t){
		.name = format("material %d", id),
		.color = {rand() % 255, rand() % 255, rand() % 255}};
	return id;
}

void generate_some_materials(void)
{
	for (int i = 0; i < 6; i++)
	{
		generate_one_material();
	}
}

material_t* get_material(material_id_t id)
{
	assert(0 <= id && id < g_material_da_len);
	return &g_material_da[id];
}

material_id_t rand_material(void)
{
	assert(g_material_da_len > 0);
	return rand() % g_material_da_len;
}
