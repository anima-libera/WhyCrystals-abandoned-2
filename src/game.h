
#ifndef WHYCRYSTALS_HEADER_GAME_
#define WHYCRYSTALS_HEADER_GAME_

#include "rendering.h"

extern int g_turn_number;

/* Section `material_t`. */

enum material_type_t
{
	MATERIAL_HARD,
	MATERIAL_VEGETAL,
	MATERIAL_TISSUE,
	MATERIAL_LIQUID,
};
typedef enum material_type_t material_type_t;

char const* material_type_name(material_type_t type);

struct material_t
{
	char* name;
	material_type_t type;
	rgb_t primary_color;
	rgb_t secondary_color;
};
typedef struct material_t material_t;

/* The index of a material in the global dynamic array of all the materials. */
typedef int material_id_t;

material_id_t generate_material(material_type_t type);
void generate_some_materials(void);

material_t* get_material(material_id_t id);

material_id_t rand_material(material_type_t type);

#endif /* WHYCRYSTALS_HEADER_GAME_ */
