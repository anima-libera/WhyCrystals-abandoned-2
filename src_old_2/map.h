
#ifndef WHYCRYSTALS_HEADER_MAP_
#define WHYCRYSTALS_HEADER_MAP_

/* Tile coordinates. */
struct tc_t
{
	int x, y;
};
typedef struct tc_t tc_t;

#include "objs.h"

#ifdef OBJ_IS_DEFINED

struct tile_t
{
	objid_t objid;
};
typedef struct tile_t tile_t;

#endif /* OBJ_IS_DEFINED */

#endif /* WHYCRYSTALS_HEADER_MAP_ */
