#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>
#include "Universe-data.h"

void universe_display_init(SDL_Window **out_win, SDL_Renderer **out_rend, int universe_dimensions);
void draw_universe(int initial_trash, int n_of_planets, struct planet_stucture planets[], struct trash_stucture trash[], SDL_Renderer* rend);
void destroy_universe(SDL_Renderer* rend, SDL_Window* win);
Uint32 SDL_ColorToUint(SDL_Color c);

#endif 