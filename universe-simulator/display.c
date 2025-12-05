#include <SDL2/SDL.h>
#include "SDL2/SDL_pixels.h"
#include "SDL2/SDL2_gfxPrimitives.h"
#include <stdlib.h>
#include <stdio.h>
#include "display.h"

Uint32 SDL_ColorToUint(SDL_Color c){
    return (Uint32)((c.a << 24) + (c.b << 16) + (c.g << 8)+ (c.r << 0));
}

//SDL window and renderer initialization
void universe_display_init(SDL_Window **out_win, SDL_Renderer **out_rend, int universe_dimensions)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        *out_win = NULL;
        *out_rend = NULL;
        return;
    }
    *out_win = SDL_CreateWindow("Universe-Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, universe_dimensions, universe_dimensions, 0);
    *out_rend = SDL_CreateRenderer(*out_win, -1, SDL_RENDERER_ACCELERATED);
}

//The function that draws the universe renders the planets first, then the trash. This way, the trash will appear "on top" of the trash
void draw_universe(int initial_trash, int n_of_planets, struct planet_stucture planets[], struct trash_stucture trash[], SDL_Renderer* rend)
{
    SDL_Color backgroud_color = {255,255,255,0};
    SDL_Color planet_color = {0,0,255,255};
    SDL_Color trash_color = {255,0,0,255};
    SDL_Color recycle_planet_color = {0,255,0,255};

    SDL_SetRenderDrawColor(rend, backgroud_color.r, backgroud_color.g, backgroud_color.b, backgroud_color.a);
    SDL_RenderClear(rend);

    for (int i = 0; i < n_of_planets; i++) 
    {
        if (planets[i].isrecycle)
        {
            filledCircleColor(rend, (int)planets[i].x, (int)planets[i].y, PLANET_RADIUS, SDL_ColorToUint(recycle_planet_color));
        } 
        else
        {
            filledCircleColor(rend, (int)planets[i].x, (int)planets[i].y, PLANET_RADIUS, SDL_ColorToUint(planet_color));
        }
    }

    
    for (int i = 0; i < initial_trash; i++) 
    {
        if (trash[i].status)
        {
            filledCircleColor(rend, (int)trash[i].x, (int)trash[i].y, 5, SDL_ColorToUint(trash_color));
        }
        
    }

    SDL_RenderPresent(rend);
}

//Once the simulation ends, close the SDL window and renderer
void destroy_universe(SDL_Renderer* rend, SDL_Window* win)
{
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
}