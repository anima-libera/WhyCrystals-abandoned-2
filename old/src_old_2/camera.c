
#include "camera.h"
#include "window.h"
#include "utils.h"

int g_tile_side_pixels = 64;

/* What are the screen coordinates of top left corner of the tile at (0, 0) ? */
sc_t g_tc_sc_offset = {0};
/* The fractional components of the values of `g_tc_sc_offset` used to make the camera
 * more precise (avoiding big rounding errors). */
float g_tc_sc_offset_frac_x = 0.0f, g_tc_sc_offset_frac_y = 0.0f;

sc_t tc_to_sc(tc_t tc)
{
	return (sc_t){
		tc.x * g_tile_side_pixels + g_tc_sc_offset.x,
		tc.y * g_tile_side_pixels + g_tc_sc_offset.y};
}

sc_t tc_to_sc_center(tc_t tc)
{
	return (sc_t){
		tc.x * g_tile_side_pixels + g_tc_sc_offset.x + g_tile_side_pixels / 2,
		tc.y * g_tile_side_pixels + g_tc_sc_offset.y + g_tile_side_pixels / 2};
}

tc_t sc_to_tc(sc_t sc)
{
	tc_t tc = {
		(sc.x - g_tc_sc_offset.x) / g_tile_side_pixels,
		(sc.y - g_tc_sc_offset.y) / g_tile_side_pixels};
	if ((sc.x - g_tc_sc_offset.x) < 0)
	{
		tc.x--;
	}
	if ((sc.y - g_tc_sc_offset.y) < 0)
	{
		tc.y--;
	}
	return tc;
}

void map_zoom(int zoom)
{
	/* Save the precise position in the map (tc coordinate space)
	 * where the mouse is pointing (before zooming) so that we can
	 * move the map after zooming to put that position under the
	 * mouse again (so that the zoom is on the cursor and not on
	 * the top left corner of the tile at (0, 0)). */
	sc_t mouse;
	SDL_GetMouseState(&mouse.x, &mouse.y);
	float old_float_tc_x =
		(float)(mouse.x - g_tc_sc_offset.x) / (float)g_tile_side_pixels;
	float old_float_tc_y =
		(float)(mouse.y - g_tc_sc_offset.y) / (float)g_tile_side_pixels;
	
	/* Zoom. */
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL])
	{
		/* Holding the left control key makes the zoom be pixel-per-pixel. */
		g_tile_side_pixels += zoom;
		if (g_tile_side_pixels <= 0)
		{
			g_tile_side_pixels = 1;
		}
	}
	else
	{
		/* Not holding the left control key makes the zoom be so that each pixel
		 * from a sprite is exactly mapped on an integer number of pixels on the
		 * screen. This is a sane default for now. */
		if (g_tile_side_pixels % 16 != 0)
		{
			if (zoom < 0)
			{
				g_tile_side_pixels -= g_tile_side_pixels % 16;
				zoom++;
			}
			else if (zoom > 0)
			{
				g_tile_side_pixels -= g_tile_side_pixels % 16;
				g_tile_side_pixels += 16;
				zoom--;
			}
		}
		g_tile_side_pixels += zoom * 16;
		int const max_side = min(g_window_w, g_window_h) / 2;
		if (g_tile_side_pixels < 16)
		{
			g_tile_side_pixels = 16;
		}
		else if (g_tile_side_pixels > max_side)
		{
			g_tile_side_pixels = max_side - max_side % 16;
		}
	}

	/* Moves the map to make sure that the precise point on the map pointed to
	 * by the mouse is still under the mouse after. */
	float new_float_tc_x =
		(float)(mouse.x - g_tc_sc_offset.x) / (float)g_tile_side_pixels;
	float new_float_tc_y =
		(float)(mouse.y - g_tc_sc_offset.y) / (float)g_tile_side_pixels;
	g_tc_sc_offset_frac_x +=
		(new_float_tc_x - old_float_tc_x) * (float)g_tile_side_pixels;
	g_tc_sc_offset.x += floorf(g_tc_sc_offset_frac_x);
	g_tc_sc_offset_frac_x -= floorf(g_tc_sc_offset_frac_x);
	g_tc_sc_offset_frac_y +=
		(new_float_tc_y - old_float_tc_y) * (float)g_tile_side_pixels;
	g_tc_sc_offset.y += floorf(g_tc_sc_offset_frac_y);
	g_tc_sc_offset_frac_y -= floorf(g_tc_sc_offset_frac_y);
}

void handle_window_resize(int new_w, int new_h)
{
	/* Save the precise position in the map (tc coordinate space) where the
	 * center of the window (before resizing the window) is so that we can
	 * move the map after resizing the window to put that position at the
	 * center again (so that the resizing is more pleasing to the eye). */
	sc_t old_center = {g_window_w / 2, g_window_h / 2};
	float old_float_tc_x =
		(float)(old_center.x - g_tc_sc_offset.x) / (float)g_tile_side_pixels;
	float old_float_tc_y =
		(float)(old_center.y - g_tc_sc_offset.y) / (float)g_tile_side_pixels;
	
	g_window_w = new_w;
	g_window_h = new_h;

	/* Moves the map to make sure that the precise point on the map at
	 * the old center is still at the center after the resizing. */
	sc_t new_center = {g_window_w / 2, g_window_h / 2};
	float new_float_tc_x =
		(float)(new_center.x - g_tc_sc_offset.x) / (float)g_tile_side_pixels;
	float new_float_tc_y =
		(float)(new_center.y - g_tc_sc_offset.y) / (float)g_tile_side_pixels;
	g_tc_sc_offset_frac_x +=
		(new_float_tc_x - old_float_tc_x) * (float)g_tile_side_pixels;
	g_tc_sc_offset.x += floorf(g_tc_sc_offset_frac_x);
	g_tc_sc_offset_frac_x -= floorf(g_tc_sc_offset_frac_x);
	g_tc_sc_offset_frac_y +=
		(new_float_tc_y - old_float_tc_y) * (float)g_tile_side_pixels;
	g_tc_sc_offset.y += floorf(g_tc_sc_offset_frac_y);
	g_tc_sc_offset_frac_y -= floorf(g_tc_sc_offset_frac_y);
}
