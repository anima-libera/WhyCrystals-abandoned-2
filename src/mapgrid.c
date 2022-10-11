
#include "mapgrid.h"
#include <stdlib.h>

tile_t* mg_tile(tc_t tc)
{
	if (tc_in_rect(tc, g_mg_rect))
	{
		return &g_mg[tc.y * g_mg_rect.w + tc.x];
	}
	else
	{
		return NULL;
	}
}

tile_t* g_mg = NULL;
tc_rect_t g_mg_rect = {0, 0, -1, -1};
