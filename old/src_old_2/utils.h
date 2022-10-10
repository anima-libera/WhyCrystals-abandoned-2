
#ifndef WHYCRYSTALS_HEADER_UTILS_
#define WHYCRYSTALS_HEADER_UTILS_

/* The true fundamental circle constant.
 * See tauday.com/tau-manifesto for more. */
#define TAU 6.28318530717f

inline int min(int a, int b)
{
	return a < b ? a : b;
}

inline int max(int a, int b)
{
	return a < b ? b : a;
}

#include <assert.h>
#include <stdlib.h>

/* Changes the length of a dynamic vector.
 * Here is an example of the intended use:
 *    int da_len, da_cap;
 *    elem_t* da;
 *    ...
 *    DA_LENGTHEN(da_len += 1, da_cap, da, elem_t);
 * Notice how the first argument must be an expression that evaluates
 * to the new length of the dynamic array (and if it has the side effect
 * of updating the length variable it is even better).
 * Do NOT fall into the trap of POST-incrementing the length (which will
 * not evaluate to the new desired length as it should).
 * The new elements are not initialized. */
#define DA_LENGTHEN(len_expr_, cap_, array_ptr_, elem_type_) \
	do { \
		int const len_ = len_expr_; \
		if (len_ > cap_) \
		{ \
			int const new_cap = max(len_, ((float)cap_ + 2.3f) * 1.3f); \
			elem_type_* new_array = realloc(array_ptr_, new_cap * sizeof(elem_type_)); \
			assert(new_array != NULL); \
			array_ptr_ = new_array; \
			cap_ = new_cap; \
		} \
	} while (0)

/* The cool modulo operator, the one that gives 7 for -3 mod 10. */
inline int cool_mod(int a, int b)
{
	assert(b > 0);
	if (a >= 0)
	{
		return a % b;
	}
	else
	{
		return (b - ((-a) % b)) % b;
	}
}

#endif /* WHYCRYSTALS_HEADER_UTILS_ */
