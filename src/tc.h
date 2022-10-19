
#ifndef WHYCRYSTALS_HEADER_TC_
#define WHYCRYSTALS_HEADER_TC_

#include <SDL2/SDL.h>
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

/* Tile coords but with float.
 * Should only be used for visual effects like particles and stuff. */
struct tcf_t
{
	float x, y;
};
typedef struct tcf_t tcf_t;

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
tm_t tm_reverse(tm_t move);
tm_t tc_diff_as_tm(tc_t src, tc_t dst);
tm_t tm_from_arrow_key(SDL_KeyCode keycode);

/* Section Bresenham. */

/* Bresenham line algorithm iterator (that respects the order:
 * the first step will be A and the last will be B).
 * It should be used as follows:
 *    bresenham_it_t it = line_bresenham_init(a_tc, b_tc);
 *    while (line_bresenham_iter(&it)) {...}
 * where the current step tc is `it.head`. */
struct bresenham_it_t
{
	tc_t a, b, head;
	int dx, dy, i, d;
	bool just_initialized;
};
typedef struct bresenham_it_t bresenham_it_t;

bresenham_it_t line_bresenham_init(tc_t a, tc_t b);
bool line_bresenham_iter(bresenham_it_t* it);

#endif /* WHYCRYSTALS_HEADER_TC_ */
