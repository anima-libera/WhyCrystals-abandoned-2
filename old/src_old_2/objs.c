
#include "objs.h"
#include "map.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void obj_draw(obj_t* obj)
{
	assert(false);
}

struct obj_entry_t
{
	bool exists;
	int generation;
	obj_t* obj;
};
typedef struct obj_entry_t obj_entry_t;

int g_objda_len = 0, g_objda_cap = 0;
obj_entry_t* g_objda = NULL;

bool objid_is_null(objid_t objid)
{
	return objid.generation == 0;
}

obj_t* obj_get(objid_t objid)
{
	assert(objid.index < g_objda_len);
	obj_entry_t* entry = &g_objda[objid.index];
	if (entry->generation == objid.generation)
	{
		return entry->obj;
	}
	else
	{
		return NULL;
	}
}

bool g_debug_obj_allocation = false;

objid_t obj_create(int obj_size)
{
	int entry_index;
	for (int i = 0; i < g_objda_len; i++)
	{
		if (!g_objda[i].exists)
		{
			entry_index = i;
			goto entry_found;
		}
	}
	DA_LENGTHEN(g_objda_len += 1, g_objda_cap, g_objda, obj_entry_t);
	if (g_debug_obj_allocation)
	{
		printf("[obj_allocation] Extended obj entry da to cap %d.\n", g_objda_cap);
	}
	entry_index = g_objda_len-1;

	entry_found:;
	obj_entry_t* entry = &g_objda[entry_index];
	entry->exists = true;
	entry->generation++;
	assert(entry->generation > 0);
	entry->obj = calloc(1, obj_size);
	if (g_debug_obj_allocation)
	{
		printf("[obj_allocation] Allocated obj of size %d.\n", obj_size);
	}
	return (objid_t){.index = entry_index, .generation = entry->generation};
}
