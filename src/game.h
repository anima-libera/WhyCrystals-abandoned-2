
#ifndef WHYCRYSTALS_HEADER_GAME_
#define WHYCRYSTALS_HEADER_GAME_

#include "rendering.h"

extern int g_turn_number;

/* Section `material_t`. */

struct material_t
{
	char* name;
	rgb_t color;
};
typedef struct material_t material_t;

/* The index of a material in the global dynamic array of all the materials. */
typedef int material_id_t;

material_id_t generate_one_material(void);
void generate_some_materials(void);

material_t* get_material(material_id_t id);

material_id_t rand_material(void);

#endif /* WHYCRYSTALS_HEADER_GAME_ */
