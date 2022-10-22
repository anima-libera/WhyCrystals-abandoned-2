
#ifndef WHYCRYSTALS_HEADER_GAMELOOP_
#define WHYCRYSTALS_HEADER_GAMELOOP_

#include <stdbool.h>

enum input_event_direction_t
{
	INPUT_EVENT_DIRECTION_UP,
	INPUT_EVENT_DIRECTION_RIGHT,
	INPUT_EVENT_DIRECTION_DOWN,
	INPUT_EVENT_DIRECTION_LEFT,
};
typedef enum input_event_direction_t input_event_direction_t;

struct game_state_t
{
	void (*draw_layer)(void);
	void (*handle_input_event_escape)(void);
	void (*handle_input_event_direction)(input_event_direction_t input_event_direction);
	void (*handle_input_event_letter_key)(char letter);
	void (*handle_input_event_debugging_letter_key)(char letter);
};
typedef struct game_state_t game_state_t;

void push_game_state(game_state_t game_state);
void pop_game_state(void);

/* Has the player spawned and stuff be displayed to the user ? */
extern bool g_game_has_started;

extern int g_turn_number;

/* Time since the beginning of the game loop, in milliseconds. */
extern int g_game_time;

/* FPS estimated during a recent iteration. */
extern int g_fps_in_some_recent_iteration;

extern bool g_should_restart;

void enter_gameloop(void);

#endif /* WHYCRYSTALS_HEADER_GAMELOOP_ */
