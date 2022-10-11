
#ifndef WHYCRYSTALS_HEADER_MAPGRID_
#define WHYCRYSTALS_HEADER_MAPGRID_

#include "objects.h"
#include "tc.h"
#include <stdbool.h>

struct tile_t
{
	oid_da_t oid_da;
	bool is_path;
	int vision;
};
typedef struct tile_t tile_t;

tile_t* mg_tile(tc_t tc);

/* Map grid. */
extern tile_t* g_mg;
extern tc_rect_t g_mg_rect;

#endif /* WHYCRYSTALS_HEADER_MAPGRID_ */
