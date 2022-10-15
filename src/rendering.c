
#include "rendering.h"
#include <stdlib.h>
#include <assert.h>

int g_window_w = 1200, g_window_h = 600;
SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;

TTF_Font* g_font_table[FONT_NUMBER] = {0};

rgba_t rgb_to_rgba(rgb_t rgb, uint8_t alpha)
{
	return (rgba_t){rgb.r, rgb.g, rgb.b, alpha};
}

rgb_t g_color_bg_shadow =   {  5,  30,  25};
rgb_t g_color_bg =          { 10,  40,  35};
rgb_t g_color_bg_bright =   { 20,  80,  70};
rgb_t g_color_white =       {180, 220, 200};
rgb_t g_color_yellow =      {230, 240,  40};
rgb_t g_color_red =         {230,  40,  35};
rgb_t g_color_green =       { 10, 240,  45};
rgb_t g_color_light_green = { 40, 240,  90};
rgb_t g_color_dark_green =  { 10, 160,  40};
rgb_t g_color_cyan =        { 20, 200, 180};

SDL_Texture* text_to_texture(char const* text, rgba_t color, font_t font)
{
	assert(0 <= font && font < FONT_NUMBER);
	SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
	SDL_Surface* surface = TTF_RenderText_Solid(g_font_table[font], text, sdl_color);
	assert(surface != NULL);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(texture != NULL);
	SDL_FreeSurface(surface);
	return texture;
}

void draw_text_rect(char const* text, rgba_t color, font_t font, SDL_Rect rect)
{
	SDL_Texture* texture = text_to_texture(text, color, font);
	SDL_RenderCopy(g_renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
}

void draw_text_sc(char const* text, rgba_t color, font_t font, sc_t sc)
{
	SDL_Texture* texture = text_to_texture(text, color, font);
	SDL_Rect rect = {.x = sc.x, .y = sc.y};
	SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
	SDL_RenderCopy(g_renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
}

int g_tile_w = 30, g_tile_h = 30;

void camera_set(camera_t* camera, tc_t target_tc)
{
	/* Make the target_tc */
	camera->x = (float)target_tc.x * (float)g_tile_w + 0.5f * (float)g_tile_w;
	camera->y = (float)target_tc.y * (float)g_tile_h + 0.5f * (float)g_tile_h;
}

void camera_move_smoothly(camera_t* camera, tc_t target_tc, float move_speed)
{
	/* Move the camera smoothly to keep the targeted tile centered. */
	camera_t target;
	camera_set(&target, target_tc);
	camera->x += (target.x - camera->x) * move_speed;
	camera->y += (target.y - camera->y) * move_speed;
}

SDL_Rect camera_tc_rect(camera_t camera, tc_t tc)
{
	return (SDL_Rect){
		tc.x * g_tile_w + g_window_w / 2 - (int)camera.x,
		tc.y * g_tile_h + g_window_h / 2 - (int)camera.y,
		g_tile_w, g_tile_h};
}
