
#ifndef WHYCRYSTALS_HEADER_OBJS_
#define WHYCRYSTALS_HEADER_OBJS_

#include "map.h"

#define OBJ_IS_DEFINED

enum obj_type_t
{
	OBJ_UNIT_CAPE,
	
	OBJ_NUMBER
};
typedef enum obj_type_t obj_type_t;

struct obj_header_t
{
	obj_type_t type;
	tc_t tc;
};
typedef struct obj_header_t obj_header_t;

struct obj_unit_cape_t
{
	obj_header_t header;
};
typedef struct obj_unit_cape_t obj_unit_cape_t;

union obj_t
{
	obj_type_t type;
	obj_header_t header;
	obj_unit_cape_t unit_basic;
};
typedef union obj_t obj_t;

#include <stdint.h>

/* Object ID. */
struct objid_t
{
	int32_t index;
	int32_t generation;
};
typedef struct objid_t objid_t;

#define OBJID_NULL ((objid_t){.generation = 0})

objid_t obj_create(int obj_size);
#define OBJ_ALLOC(obj_type_) obj_alloc(sizeof(obj_type_))

#endif /* WHYCRYSTALS_HEADER_OBJS_ */
