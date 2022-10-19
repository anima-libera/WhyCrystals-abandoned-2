
#ifndef WHYCRYSTALS_HEADER_LAWS_
#define WHYCRYSTALS_HEADER_LAWS_

#include "objects.h"

struct law_t
{
	const char* name;
	void (*function)(oid_t oid);
};
typedef struct law_t law_t;

extern law_t* g_law_da;
extern int g_law_da_len, g_law_da_cap;

void register_laws(void);
void apply_laws(void);

#endif /* WHYCRYSTALS_HEADER_LAWS_ */
