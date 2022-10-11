
#include "tc.h"
#include <stdlib.h>

bool tc_eq(tc_t a, tc_t b)
{
	return a.x == b.x && a.y == b.y;
}

bool tc_in_rect(tc_t tc, tc_rect_t rect)
{
	return
		rect.x <= tc.x && tc.x < rect.x + rect.w &&
		rect.y <= tc.y && tc.y < rect.y + rect.h;
}

tm_t rand_tm_one(void)
{
	return TM_ONE_ALL[rand() % 4];
}

bool tm_one_orthogonal(tm_t a, tm_t b)
{
	return !((a.x != 0 && b.x != 0) || (a.y != 0 && b.y != 0));
}

tc_t tc_add_tm(tc_t tc, tm_t move)
{
	return (tc_t){.x = tc.x + move.x, .y = tc.y + move.y};
}
