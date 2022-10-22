
#include "gameloop.h"
#include "utils.h"
#include "rendering.h"
#include "game.h"
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

game_state_t* g_game_state_da = NULL;
int g_game_state_da_len = 0, g_game_state_da_cap = 0;

void push_game_state(game_state_t game_state)
{
	DA_LENGTHEN(g_game_state_da_len += 1, g_game_state_da_cap, g_game_state_da, game_state_t);
	g_game_state_da[g_game_state_da_len-1] = game_state;
}

void pop_game_state(void)
{
	assert(g_game_state_da_len > 0);
	g_game_state_da_len--;
}

game_state_t* top_game_state(void)
{
	if (g_game_state_da_len > 0)
	{
		return &g_game_state_da[g_game_state_da_len-1];
	}
	else
	{
		return NULL;
	}
}

int g_game_time = 0;
int g_fps_in_some_recent_iteration = 0;

input_event_direction_t input_event_direction_from_keycode(SDL_Keycode keycode)
{
	switch (keycode)
	{
		case SDLK_UP:    return INPUT_EVENT_DIRECTION_UP;
		case SDLK_RIGHT: return INPUT_EVENT_DIRECTION_RIGHT;
		case SDLK_DOWN:  return INPUT_EVENT_DIRECTION_DOWN;
		case SDLK_LEFT:  return INPUT_EVENT_DIRECTION_LEFT;
		default:         assert(false); exit(EXIT_FAILURE);
	}
}

bool g_should_restart = false;

void enter_gameloop(void)
{
	int start_loop_time = SDL_GetTicks();
	int iteration_duration = 0; /* In milliseconds. */
	int iteration_number = 0;

	printf("Enter gameloop, game starts\n");
	g_game_has_started = true;
	while (true)
	{
		int start_iteration_time = SDL_GetTicks();
		g_game_time = start_iteration_time - start_loop_time;

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			game_state_t* game_state = top_game_state();
			if (game_state == NULL)
			{
				goto exit_gameloop;
			}

			switch (event.type)
			{
				case SDL_QUIT:
					goto exit_gameloop;
				break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_RESIZED:
							SDL_GetRendererOutputSize(g_renderer, &g_window_w, &g_window_h);
						break;
					}
				break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LSHIFT] &&
								SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL])
							{
								printf("Restart (beware the bugs)\n");
								g_should_restart = true;
								goto exit_gameloop;
							}
							if (game_state->handle_input_event_escape != NULL)
							{
								game_state->handle_input_event_escape();
							}
							else
							{
								pop_game_state();
							}
						break;
						case SDLK_UP:
						case SDLK_RIGHT:
						case SDLK_DOWN:
						case SDLK_LEFT:
							if (game_state->handle_input_event_direction != NULL)
							{
								game_state->handle_input_event_direction(
									input_event_direction_from_keycode(event.key.keysym.sym));
							}
						break;
						default:
							if (SDLK_a <= event.key.keysym.sym && event.key.keysym.sym <= SDLK_z &&
								SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LSHIFT] &&
								SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL] &&
								game_state->handle_input_event_debugging_letter_key != NULL)
							{
								game_state->handle_input_event_debugging_letter_key(
									event.key.keysym.sym);
							}
						break;
					}
				break;
			}
		}

		if (g_game_state_da_len == 0)
		{
			goto exit_gameloop;
		}

		SDL_SetRenderDrawColor(g_renderer,
			g_color_bg_shadow.r, g_color_bg_shadow.g, g_color_bg_shadow.b, 255);
		SDL_RenderClear(g_renderer);

		for (int i = 0; i < g_game_state_da_len; i++)
		{
			if (g_game_state_da[i].draw_layer != NULL)
			{
				g_game_state_da[i].draw_layer();
			}
		}

		SDL_RenderPresent(g_renderer);

		iteration_number++;

		int end_iteration_time = SDL_GetTicks();
		iteration_duration = end_iteration_time - start_iteration_time;

		if (iteration_number % 20 == 0)
		{
			g_fps_in_some_recent_iteration =
				iteration_duration == 0 ? 0.0f : 1000 / iteration_duration;
		}
	}

	exit_gameloop:
	printf("Exit gameloop\n");
	if (g_game_state_da_len > 0)
	{
		printf("Game state stack is not empty\n");
	}
}
