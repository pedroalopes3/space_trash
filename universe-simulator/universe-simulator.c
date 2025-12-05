#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <libconfig.h>
#include "universe-data.h"
#include "physics-rules.h"
#include "display.h"


Uint32 timer_callback(Uint32 interval, void* param){
    SDL_Event timer_event;
    SDL_zero(timer_event); 
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 2;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}


int main(int argc, char *argv[]){
    
    // Read and save variables from the config file
    config_t cfg;
    config_init(&cfg);

    if(!config_read_file(&cfg, "universe-simulator.conf")) 
    {
        fprintf(stderr, "Config file error\n");
        config_destroy(&cfg);
        return 1;
    }

    int universe_dimensions;
    config_lookup_int(&cfg, "universe_dimensions", &universe_dimensions);

    int n_of_planets;
    config_lookup_int(&cfg, "n_of_planets", &n_of_planets);

    int max_trash;
    config_lookup_int(&cfg, "max_trash", &max_trash);

    int initial_trash;
    config_lookup_int(&cfg, "initial_trash", &initial_trash);

    config_destroy(&cfg);

    //Allocate space for the planet and trash structures
    struct planet_stucture *planets = malloc(sizeof(*planets) * n_of_planets);
    struct trash_stucture *trash = malloc(sizeof(*trash) * max_trash);
    if (!planets || !trash) 
    {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    universe_data_init(planets, n_of_planets, trash, initial_trash, universe_dimensions, max_trash);

    SDL_Window *win = NULL;
    SDL_Renderer *rend = NULL;
    universe_display_init(&win, &rend, universe_dimensions);
    if (!win || !rend) 
    {
        fprintf(stderr, "Failed to create window/renderer\n");
        free(planets);
        free(trash);
        return 1;
    }

    int close = 0;

    SDL_TimerID timer_id = 0;
    timer_id = SDL_AddTimer(10,(SDL_TimerCallback)timer_callback, NULL);
    SDL_TimerID trash_timer_id = 0;
    int trash_count = initial_trash;
    while (!close) {
        
        if (trash_count >= max_trash) 
        {
            close = 1;
            printf("Universe imploded from too much trash\n");
            break;
        }

        SDL_Event event;
        SDL_WaitEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                close = 1; break;
            case SDL_USEREVENT:
                if (event.user.code == 2) 
                {
                    //Main Universe loop: every 10 ms the timer triggers this event, which updates the physics state of the universe
                    //before drawing it again and updating the trash count
                    physics_update(planets, n_of_planets, trash, max_trash, universe_dimensions);
                    draw_universe(max_trash, n_of_planets, planets, trash, rend);
                    trash_count = update_trash_count(trash, max_trash);
                }
                break;
            default:
                break;
        }
    }

    destroy_universe(rend, win);
    free(planets);
    free(trash);
    return 0;
}