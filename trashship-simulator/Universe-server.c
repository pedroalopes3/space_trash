// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL2/SDL_pixels.h"   
#include <libconfig.h>

#include "Universe-data.h"
#include "Display.h"
#include "Communication.h"
#include <zmq.h>




// Function Declaration
int read_keys();
void correct_position(float *coord, int universe_dimensions);

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

    int ship_capacity;
    config_lookup_int(&cfg, "ship_capacity", &ship_capacity);

    config_destroy(&cfg);
    // Initialize SDL
    //
    // returns zero on success else non-zero
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize communication with server

   CommHandle *comm = comm_server_init();
    if (!comm) {
        fprintf(stderr, "Failed to init server communication\n");
        return -1;
    }


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
        return 1;
    }

    int close = 0;
    int key_pressed = 0;
    // next available client ID
    int client_id = 0;
    draw_universe(initial_trash, n_of_planets, planets, trash, rend,ship);

    printf("Server ready\n");

    // Main loop
    while (!close) {
        SDL_Event event;

        // Handle SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                key_pressed = 1;  // Set flag to exit loop
            }
        }

        // Receive messages
        uint8_t buffer[1024];
        int len = comm_server_recv(comm, buffer, sizeof(buffer));

        if (len > 0) {
            if (buffer[len-1] == '\0' && strcmp((char *)buffer, "HELLO") == 0) {
                printf("Server: received HELLO\n");
                comm_server_send(comm, "HELLO_RESPONSE", 14);
            } else {
                join_received_message(comm, buffer, len, &client_id);
            }
        }

        // Other operations
        check_collisions(planets, n_of_planets, trash, max_trash, universe_dimensions, ship, ship_capacity);
        for (int i = 0; i < n_of_planets; i++) {
            correct_position(&ship[i].x, universe_dimensions);
            correct_position(&ship[i].y, universe_dimensions);
        }

        // Draw the updated universe
        draw_universe(initial_trash, n_of_planets, planets, trash, rend, ship);

        if (key_pressed == 1) {
            close = 1;  // Close the server if SDL quit event is triggered
        }
        usleep(10000);  /* small sleep to avoid 100% CPU */
    }

    // Cleanup
    comm_close(comm);
    destroy_universe(rend, win);
    free(planets);
    free(trash);
    free(ship);
    return 0;
}

void correct_position(float *coord, int universe_dimensions) 
{
    if (*coord < 0) *coord += universe_dimensions;
    if (*coord >= universe_dimensions) *coord -= universe_dimensions;
}



