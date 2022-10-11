
#ifndef WHYCRYSTALS_HEADER_TC_
#define WHYCRYSTALS_HEADER_TC_

#include <stdbool.h>

/* Tile coords. */
struct tc_t
{
	int x, y;
};
typedef struct tc_t tc_t;

bool tc_eq(tc_t a, tc_t b);

struct tc_rect_t
{
	int x, y, w, h;
};
typedef struct tc_rect_t tc_rect_t;

bool tc_in_rect(tc_t tc, tc_rect_t rect);

/* Tile move. Represents a move on the grid rather than a tile. */
typedef tc_t tm_t;
#define TM_UP    (tm_t){.x =  0, .y = -1}
#define TM_RIGHT (tm_t){.x = +1, .y =  0}
#define TM_DOWN  (tm_t){.x =  0, .y = +1}
#define TM_LEFT  (tm_t){.x = -1, .y =  0}
#define TM_ONE_ALL (tm_t[]){TM_UP, TM_RIGHT, TM_DOWN, TM_LEFT}

tm_t rand_tm_one(void);
bool tm_one_orthogonal(tm_t a, tm_t b);
tc_t tc_add_tm(tc_t tc, tm_t move);

#endif /* WHYCRYSTALS_HEADER_TC_ */
