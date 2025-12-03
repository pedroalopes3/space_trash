// Libraries
#include <stdio.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL2/SDL_pixels.h"  
#include <stdlib.h>
#include <time.h>
#include <libconfig.h>

#include "Universe-data.h"
#include "Display.h" 
#include "Communication.h"


// Function Declaration
int read_keys();   

// Main Function
int main(void)       
{

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

    struct planet_stucture *planets = malloc(sizeof(*planets) * n_of_planets);
    struct trash_stucture *trash = malloc(sizeof(*trash) * initial_trash);
    if (!planets || !trash) 
    {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    universe_data_init(planets, n_of_planets, trash, initial_trash, universe_dimensions);

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


    // Initialize communication with server

   CommHandle *comm = comm_server_init();
    if (!comm) {
        fprintf(stderr, "Failed to init server communication\n");
        return -1;
    }


    int close = 0;
    int key_pressed = 0;
    printf("antes\n");

    draw_universe(initial_trash, n_of_planets, planets, trash, rend);

    while (!close) {

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                key_pressed = 1;  // Handle close button event
             }
        }
       
        char buffer[128];
        
        // Receive request
        int n = comm_server_recv(comm, buffer, sizeof(buffer)-1,10);
        if (n>0){
            buffer[n] = '\0';
            printf("Server request: %s\n", buffer);
                    
            //Send reply
            const char *reply = "thanks!!!!";
            comm_server_send(comm, reply, strlen(reply));   

            draw_universe(initial_trash, n_of_planets, planets, trash, rend);
        }
            
        
        if (key_pressed == 1){
            close = 1;
        }

        SDL_Delay(10);

        }
    comm_close(comm);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

int read_keys(){
    int pressed_key_code = 0;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            pressed_key_code = 1;  // Handle close button event
        }
    }
     
    return pressed_key_code;
}
