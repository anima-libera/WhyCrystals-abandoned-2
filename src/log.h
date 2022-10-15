
#ifndef WHYCRYSTALS_HEADER_LOG_
#define WHYCRYSTALS_HEADER_LOG_

#include "rendering.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

struct log_entry_t
{
	/* Can be NULL, in which case the entry is a turn separator
	 * (between the turn `turn_number` and the turn `turn_number+1`). */
	char* text;
	/* The number of the turn during which this entry was logged.
	 * This is useful to make the entry disappear after a few turns. */
	int turn_number;
	/* When the entry is to disappear, it is given a countdown for a fading effect. */
	int time_remaining;
	#define LOG_ENTRY_TIME_REMAINING_INIT 60
	/* This is the time at which the entry was logged, for an appearing effect. */
	int time_created;
};
typedef struct log_entry_t log_entry_t;

extern int g_log_len, g_log_cap;
extern log_entry_t* g_log_da;

/* Returns an allocated string. */
char* format(char* format, ...);
char* vformat(char* format, va_list va);

void log_text(char* format, ...);
void log_turn_seperator(void);

void draw_log(void);

#endif /* WHYCRYSTALS_HEADER_LOG_ */
