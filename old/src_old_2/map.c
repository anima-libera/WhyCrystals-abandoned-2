
#include "map.h"
#include "utils.h"
#include "embedded.h"
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define CHUNK_SIDE 16

struct chunk_t
{
	tc_t top_left_tc;
	tile_t* tiles;
};
typedef struct chunk_t chunk_t;

/* Tiles are stored in fixed-sized chunks, and chunks are stored in a dynaic array.
 * New chunks can be generated without having to move the existing tiles (so there
 * is no need to worry about the validity of pointers to tiles). */
int g_chunk_da_len = 0, g_chunk_da_cap = 0;
chunk_t* g_chunk_da = NULL;

bool tc_in_map(tc_t tc)
{
	for (int i = 0; i < g_chunk_da_len; i++)
	{
		chunk_t* chunk = &g_chunk_da[i];
		if (chunk->top_left_tc.x <= tc.x && tc.x < chunk->top_left_tc.x + CHUNK_SIDE &&
			chunk->top_left_tc.y <= tc.y && tc.y < chunk->top_left_tc.y + CHUNK_SIDE)
		{
			return true;
		}
	}
	return false;
}

void generate_tile(tile_t* tile, tc_t tc)
{
	tile->objid = OBJID_NULL;

	if (tc.x == 0 && tc.y == 0)
	{
		tile->objid = OBJ_ALLOC(obj_unit_cape_t);
	}
}

bool g_some_chunks_are_new = false;

tile_t* map_tile(tc_t tc)
{
	/* Search in the already generated chunks first. */
	for (int i = 0; i < g_chunk_da_len; i++)
	{
		chunk_t* chunk = &g_chunk_da[i];
		if (chunk->top_left_tc.x <= tc.x && tc.x < chunk->top_left_tc.x + CHUNK_SIDE &&
			chunk->top_left_tc.y <= tc.y && tc.y < chunk->top_left_tc.y + CHUNK_SIDE)
		{
			int inchunk_x = tc.x - chunk->top_left_tc.x;
			int inchunk_y = tc.y - chunk->top_left_tc.y;
			return &chunk->tiles[inchunk_y * CHUNK_SIDE + inchunk_x];
		}
	}
	/* It seems that the requested tile was not generated yet,
	 * so we generate the chunk that contains it now. */
	g_some_chunks_are_new = true;
	DA_LENGTHEN(g_chunk_da_len += 1, g_chunk_da_cap, g_chunk_da, chunk_t);
	chunk_t* chunk = &g_chunk_da[g_chunk_da_len-1];
	chunk->top_left_tc.x = tc.x - cool_mod(tc.x, CHUNK_SIDE);
	chunk->top_left_tc.y = tc.y - cool_mod(tc.y, CHUNK_SIDE);
	#if 0
	printf("Generate chunk (%d~%d, %d~%d) for tile (%d, %d)\n",
		chunk->top_left_tc.x, chunk->top_left_tc.x + CHUNK_SIDE - 1,
		chunk->top_left_tc.y, chunk->top_left_tc.y + CHUNK_SIDE - 1,
		tc.x, tc.y);
	#endif
	chunk->tiles = calloc(CHUNK_SIDE * CHUNK_SIDE, sizeof(tile_t));
	for (int i = 0; i < CHUNK_SIDE * CHUNK_SIDE; i++)
	{
		tc_t tc = {
			.x = chunk->top_left_tc.x + i % CHUNK_SIDE,
			.y = chunk->top_left_tc.y + i / CHUNK_SIDE};
		generate_tile(&chunk->tiles[i], tc);
	}
	int inchunk_x = tc.x - chunk->top_left_tc.x;
	int inchunk_y = tc.y - chunk->top_left_tc.y;
	return &chunk->tiles[inchunk_y * CHUNK_SIDE + inchunk_x];
}
