
#include "tc.h"
#include <stdlib.h>
#include <assert.h>

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

tm_t tm_reverse(tm_t move)
{
    return (tm_t){.x = -move.x, .y = -move.y};
}

tm_t tc_diff_as_tm(tc_t src, tc_t dst)
{
    return (tm_t){.x = dst.x - src.x, .y = dst.y - src.y};
}

tm_t tm_from_input_event_direction(input_event_direction_t input_event_direction)
{
	switch (input_event_direction)
	{
		case INPUT_EVENT_DIRECTION_UP:    return TM_UP;
		case INPUT_EVENT_DIRECTION_RIGHT: return TM_RIGHT;
		case INPUT_EVENT_DIRECTION_DOWN:  return TM_DOWN;
		case INPUT_EVENT_DIRECTION_LEFT:  return TM_LEFT;
		default:                          assert(false); exit(EXIT_FAILURE);
	}
}

/* Section Bresenham. */

bresenham_it_t line_bresenham_init(tc_t a, tc_t b)
{
	bresenham_it_t it;
	it.a = a;
	it.b = b;
	#define LINE_BRESENHAM_INIT(x_, y_) \
		it.d##x_ = abs(it.b.x_ - it.a.x_); \
		it.d##y_ = it.b.y_ - it.a.y_; \
		it.i = 1; \
		if (it.d##y_ < 0) \
		{ \
			it.i *= -1; \
			it.d##y_ *= -1; \
		} \
		it.d = (2 * it.d##y_) - it.d##x_; \
		it.head.x = it.a.x; \
		it.head.y = it.a.y;
	if (abs(b.y - a.y) < abs(b.x - a.x))
	{
		LINE_BRESENHAM_INIT(x, y)
	}
	else
	{
		LINE_BRESENHAM_INIT(y, x)
	}
	#undef LINE_BRESENHAM_INIT
	it.just_initialized = true;
	return it;
}

bool line_bresenham_iter(bresenham_it_t* it)
{
	if (it->just_initialized)
	{
		it->just_initialized = false;
		return true;
	}
	#define LINE_BRESENHAM_ITER(x_, y_) \
		if (it->head.x_ == it->b.x_) \
		{ \
			return false; \
		} \
		it->head.x_ += it->a.x_ < it->b.x_ ? 1 : -1; \
		if (it->d > 0) \
		{ \
			it->head.y_ += it->i; \
			it->d += (2 * (it->d##y_ - it->d##x_)); \
		} \
		else \
		{ \
			it->d += 2 * it->d##y_; \
		} \
		return true;
	if (abs(it->b.y - it->a.y) < abs(it->b.x - it->a.x))
	{
		LINE_BRESENHAM_ITER(x, y)
	}
	else
	{
		LINE_BRESENHAM_ITER(y, x)
	}
	#undef LINE_BRESENHAM_ITER
}
