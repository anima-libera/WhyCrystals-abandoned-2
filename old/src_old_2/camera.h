
#ifndef WHYCRYSTALS_HEADER_CAMERA_
#define WHYCRYSTALS_HEADER_CAMERA_

/* Screen coordinates. */
struct sc_t
{
	int x, y;
};
typedef struct sc_t sc_t;

#include "map.h"

sc_t tc_to_sc(tc_t tc);
sc_t tc_to_sc_center(tc_t tc);
tc_t sc_to_tc(sc_t sc);

#endif /* WHYCRYSTALS_HEADER_CAMERA_ */
