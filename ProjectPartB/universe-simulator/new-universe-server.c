#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <libconfig.h>
#include <pthread.h>
#include <zmq.h>
#include "universe-data.h"
#include "physics-rules.h"
#include "display.h"
#include "communication.h"
#include "messages.pb-c.h"


//Timer functions
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

Uint32 trash_timer_callback(Uint32 interval, void* param){
    SDL_Event timer_event;
    SDL_zero(timer_event); 
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 3;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}

Uint32 physics_timer_callback(Uint32 interval, void* param){
    SDL_Event timer_event;
    SDL_zero(timer_event); 
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 4;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}

Uint32 planet_timer_callback(Uint32 interval, void* param){
    SDL_Event timer_event;
    SDL_zero(timer_event); 
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 5;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}

int main()
{
    
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

    int ship_capacity;
    config_lookup_int(&cfg, "ship_capacity", &ship_capacity);

    config_destroy(&cfg);

    //Allocate space for the planet and trash structures
    struct planet_stucture *planets = malloc(sizeof(*planets) * n_of_planets);
    struct trash_stucture *trash = malloc(sizeof(*trash) * max_trash);
    struct trash_ship *ship = malloc(sizeof(*ship)*n_of_planets);
    if (!planets || !trash || !ship) 
    {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    universe_data_init(planets, n_of_planets, trash, initial_trash, universe_dimensions, max_trash, ship);

    SDL_Window *win = NULL;
    SDL_Renderer *rend = NULL;
    universe_display_init(&win, &rend, universe_dimensions);
    if (!win || !rend) 
    {
        fprintf(stderr, "Failed to create window/renderer\n");
        free(planets);
        free(trash);
        free(ship);
        return 1;
    }

    int close = 0;

    SDL_TimerID timer_id = 0;
    timer_id = SDL_AddTimer(33,(SDL_TimerCallback)timer_callback, NULL);
    SDL_TimerID trash_timer_id = 0;
    trash_timer_id = SDL_AddTimer(10000,(SDL_TimerCallback)trash_timer_callback, NULL);
    SDL_TimerID physics_timer_id = 0;
    physics_timer_id = SDL_AddTimer(10,(SDL_TimerCallback)physics_timer_callback, NULL);
    SDL_TimerID planet_timer_id = 0;
    planet_timer_id = SDL_AddTimer(30000,(SDL_TimerCallback)planet_timer_callback, NULL);
    
    int trash_count = initial_trash;
    
    //Load the variables that will be used by the threads
    UniverseContext *ctx = malloc(sizeof(UniverseContext));
    ctx->planets = planets;
    ctx->n_planets = n_of_planets;
    ctx->trash = trash;
    ctx->max_trash = max_trash;
    ctx->ship = ship;
    ctx->universe_dimensions = universe_dimensions;
    ctx->stop_flag = 0;
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0)
    {
        fprintf(stderr, "Failed to init mutex\n");
        free(planets);
        free(trash);
        free(ship);
        free(ctx);
        return 1;
    }

    pthread_t thread_id, broadcast_id;

    //Create two threads: one will handle communication on a REQ/REP socket, which will be responsible for input from clients
    //the other will handle communication on a PUB/SUB socket, which will broadcast the universe state to all clients, along with the info for the dashboard
    pthread_create(&thread_id, NULL, input_communication, (void*)ctx);
    pthread_create(&broadcast_id, NULL, broadcast_universe, (void*)ctx);

    while (!close) 
    {
        SDL_Event event;
        SDL_WaitEvent(&event);
        
        if (ctx->stop_flag) {
            close = 1;
            break;
        }

        switch (event.type) 
        {
            case SDL_QUIT:
                pthread_mutex_lock(&ctx->mutex);
                ctx->stop_flag = 1;  
                pthread_mutex_unlock(&ctx->mutex);
                close = 1; 
                break;
            case SDL_USEREVENT:
                if (event.user.code == 2) 
                {
                    //Draw universe every 33ms
                    pthread_mutex_lock(&ctx->mutex);
                    draw_universe(max_trash, n_of_planets, planets, trash, rend, ship);
                    pthread_mutex_unlock(&ctx->mutex);
                }

                if (event.user.code == 3)
                {
                    //add trash every 10 seconds
                    pthread_mutex_lock(&ctx->mutex);
                    if(are_all_ships_empty(ship, n_of_planets))
                    {
                    add_trash(trash, max_trash, universe_dimensions);
                    trash_count = update_trash_count(trash, max_trash);
                    }
                    pthread_mutex_unlock(&ctx->mutex);
                }

                if(event.user.code == 4)
                {
                    //update the universe every 10ms
                    pthread_mutex_lock(&ctx->mutex);
                    physics_update(planets, n_of_planets, trash, max_trash, universe_dimensions, ship, ship_capacity);
                    trash_count = update_trash_count(trash, max_trash);
                    pthread_mutex_unlock(&ctx->mutex);
                }

                if(event.user.code == 5)
                {
                    //change the recycling planet every 30 seconds
                    pthread_mutex_lock(&ctx->mutex);
                    for (int i = 0; i < n_of_planets; i++) 
                    {
                        planets[i].isrecycle = 0;
                    }                    
                    planets[rand() % n_of_planets].isrecycle = 1;
                    pthread_mutex_unlock(&ctx->mutex);
                }

                break;
            default:
                break;
        }
    }
    destroy_universe(rend, win);
    free(planets);
    free(trash);
    free(ship);
    pthread_mutex_destroy(&ctx->mutex);
    return 0;
}

