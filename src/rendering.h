
#ifndef WHYCRYSTALS_HEADER_RENDERING_
#define WHYCRYSTALS_HEADER_RENDERING_

#include "tc.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

extern int g_window_w, g_window_h;
extern SDL_Window* g_window;
extern SDL_Renderer* g_renderer;

enum font_t
{
	FONT_RG,
	FONT_TL,
	FONT_TL_SMALL,
	FONT_NUMBER
};
typedef enum font_t font_t;
extern TTF_Font* g_font_table[FONT_NUMBER];

struct rgb_t
{
	uint8_t r, g, b;
};
typedef struct rgb_t rgb_t;

struct rgba_t
{
	uint8_t r, g, b, a;
};
typedef struct rgba_t rgba_t;

rgba_t rgb_to_rgba(rgb_t rgb, uint8_t alpha);

extern rgb_t g_color_bg_shadow;
extern rgb_t g_color_bg;
extern rgb_t g_color_bg_bright;
extern rgb_t g_color_white;
extern rgb_t g_color_yellow;
extern rgb_t g_color_red;
extern rgb_t g_color_green;
extern rgb_t g_color_light_green;
extern rgb_t g_color_dark_green;
extern rgb_t g_color_cyan;
extern rgb_t g_color_blue;

/* Screen coordinates. */
struct sc_t
{
	int x, y;
};
typedef struct sc_t sc_t;

SDL_Texture* text_to_texture(char const* text, rgba_t color, font_t font);
void draw_text_rect(char const* text, rgba_t color, font_t font, SDL_Rect rect);
void draw_text_sc(char const* text, rgba_t color, font_t font, sc_t sc);
void draw_text_sc_center(char const* text, rgba_t color, font_t font, sc_t sc);

/* Tile dimensions. */
extern int g_tile_w, g_tile_h;

struct camera_t
{
	float x, y;
};
typedef struct camera_t camera_t;

void camera_set(camera_t* camera, tc_t target_tc);
void camera_move_smoothly(camera_t* camera, tc_t target_tc, float move_speed);
SDL_Rect camera_tc_rect(camera_t camera, tc_t tc);
sc_t camera_tcf(camera_t camera, tcf_t tcf);

#endif /* WHYCRYSTALS_HEADER_RENDERING_ */
