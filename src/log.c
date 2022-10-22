
#include "log.h"
#include "rendering.h"
#include "utils.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

int g_log_len = 0, g_log_cap = 0;
log_entry_t* g_log_da = NULL;

/* Returns an allocated string. */
char* format(char* format, ...)
{
	va_list va;
	va_start(va, format);
	int text_len = vsnprintf(NULL, 0, format, va);
	va_end(va);
	assert(text_len >= 0);
	char* text = malloc(text_len + 1);
	va_start(va, format);
	vsnprintf(text, text_len + 1, format, va);
	va_end(va);
	return text;
}

char* vformat(char* format, va_list va)
{
	va_list va_2;
	va_copy(va_2, va);
	int text_len = vsnprintf(NULL, 0, format, va);
	assert(text_len >= 0);
	char* text = malloc(text_len + 1);
	vsnprintf(text, text_len + 1, format, va_2);
	va_end(va_2);
	return text;
}

void log_text(char* format, ...)
{
	va_list va;
	va_start(va, format);
	char* text = vformat(format, va);
	va_end(va);

	DA_LENGTHEN(g_log_len += 1, g_log_cap, g_log_da, log_entry_t);
	for (int i = g_log_len-1; i > 0; i--)
	{
		g_log_da[i] = g_log_da[i-1];
	}
	g_log_da[0] = (log_entry_t){
		.text = text,
		.turn_number = g_turn_number,
		.time_remaining = LOG_ENTRY_TIME_REMAINING_INIT,
		.time_created = SDL_GetTicks()};
}

void log_turn_seperator(void)
{
	DA_LENGTHEN(g_log_len += 1, g_log_cap, g_log_da, log_entry_t);
	for (int i = g_log_len-1; i > 0; i--)
	{
		g_log_da[i] = g_log_da[i-1];
	}
	g_log_da[0] = (log_entry_t){
		.text = NULL,
		.turn_number = g_turn_number,
		.time_remaining = LOG_ENTRY_TIME_REMAINING_INIT,
		.time_created = SDL_GetTicks()};
}

void draw_log(void)
{
	/* Not drawing the leading separators (if any) and the redundent separators
	 * is more pleasing to the eyes, and it is easier than not producing these separators. */
	bool last_was_not_text = true;

	int y = g_window_h - 30;
	for (int i = 0; i < g_log_len; i++)
	{
		int alpha = min(255, g_log_da[i].time_remaining * (255 / LOG_ENTRY_TIME_REMAINING_INIT));
		int time_existing = SDL_GetTicks() - g_log_da[i].time_created;

		int height = min(5, time_existing);
		if (g_log_da[i].text != NULL)
		{
			SDL_Texture* texture = text_to_texture(g_log_da[i].text,
				rgb_to_rgba(g_color_white, 255), FONT_TL);
			SDL_SetTextureAlphaMod(texture, alpha);
			SDL_Rect rect = {.x = 10};
			SDL_QueryTexture(texture, NULL, NULL, &rect.w, &rect.h);
			rect.h = min(rect.h, time_existing / 3);
			rect.y = y - rect.h;
			height = rect.h;

			SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, alpha / 3);
			SDL_RenderFillRect(g_renderer, &rect);
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
			SDL_RenderCopy(g_renderer, texture, NULL, &rect);
			SDL_DestroyTexture(texture);
			SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);
		}

		if (g_turn_number > g_log_da[i].turn_number + 16)
		{
			g_log_da[i].time_remaining--;
		}

		if (g_log_da[i].text != NULL || !last_was_not_text)
		{
			y -= height;
		}

		last_was_not_text = g_log_da[i].text == NULL;
	}

	while (g_log_len > 0 && g_log_da[g_log_len-1].time_remaining <= 0)
	{
		free(g_log_da[g_log_len-1].text);
		g_log_len--;
	}
}
