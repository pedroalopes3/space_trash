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
#include "messages.pb-c.h"
#include <zmq.h>


int read_keys(int initial_trash, int n_of_planets, struct planet_stucture planets[], struct trash_stucture trash[], SDL_Renderer* rend, struct trash_ship ship[],int universe_dimensions, int ship_capacity);
static int find_ship_index_by_id(struct trash_ship ship[], int n, int id);
static int allocate_ship_slot(struct trash_ship ship[], int n, int universe_dimensions);
void correct_position(float *coord, int universe_dimensions);

int main(void)       
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
   
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

   CommHandle *comm = comm_server_init();
    if (!comm) {
        fprintf(stderr, "Failed to init server communication\n");
        return -1;
    }

    //Allocate space for the planet,trash and ship structures
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

    draw_universe(initial_trash, n_of_planets, planets, trash, rend, ship);

    while (!close) {
        int key_pressed = read_keys(initial_trash, n_of_planets, planets, trash, rend, ship, universe_dimensions, ship_capacity);
        if (key_pressed == 1){ 
            close = 1;
            break;
        }

        uint8_t buffer[256];
        int n = comm_server_recv(comm, buffer, sizeof(buffer));
        
        if (n > 0){
            // Unpack ClientMessage
            Trashship__ClientMessage *msg = trashship__client_message__unpack(NULL, n, buffer);
            if (!msg) 
            {
                fprintf(stderr, "Failed to unpack message (size=%d)\n", n);
                Trashship__ServerMessage reply_msg = TRASHSHIP__SERVER_MESSAGE__INIT;
                reply_msg.has_status = 1;
                reply_msg.status = -4;  
                size_t reply_len = trashship__server_message__get_packed_size(&reply_msg);
                uint8_t reply_buf[256];
                trashship__server_message__pack(&reply_msg, reply_buf);
                comm_server_send(comm, reply_buf, reply_len);
                continue;
            }

            uint32_t ship_id = msg->has_ship_id ? msg->ship_id : 0;
            uint32_t command = msg->has_command ? msg->command : 0;
            int32_t status = 0;
            uint32_t reply_ship_id = ship_id;

            // If the Client sends ship_id == 0, that it means it's a new client and a random ID needs to be assigned
            if (ship_id == 0) 
            {
                int slot = allocate_ship_slot(ship, n_of_planets, universe_dimensions);
                if (slot >= 0) 
                {
                    reply_ship_id = ship[slot].ID;
                } 
                else 
                {
                    status = -2; 
                }
            } 
            else 
            {
                // If the Client sends a ship_id != 0, we go through the list of ID's until we find the matching one
                int idx = find_ship_index_by_id(ship, n_of_planets, ship_id);
                if (idx < 0) 
                {
                    status = -1; 
                } 
                else 
                {
                    switch (command) {
                        case 5:  
                            ship[idx].x += 5; 
                            break;
                        case 3:  
                            ship[idx].x -= 5; 
                            break;
                        case 4:  
                            ship[idx].y += 5; 
                            break;
                        case 2:  
                            ship[idx].y -= 5; 
                            break;
                        default:
                            status = -3;  
                            break;
                    }
                    if (status == 0) {
                        correct_position(&ship[idx].x, universe_dimensions);
                        correct_position(&ship[idx].y, universe_dimensions);
                        check_collisions(planets, n_of_planets, trash, max_trash, universe_dimensions, ship, ship_capacity);
                        draw_universe(initial_trash, n_of_planets, planets, trash, rend, ship);
                    }
                }
            }

            // Send ServerMessage reply
            Trashship__ServerMessage reply_msg = TRASHSHIP__SERVER_MESSAGE__INIT;
            reply_msg.has_ship_id = 1;
            reply_msg.ship_id = reply_ship_id;
            reply_msg.has_status = 1;
            reply_msg.status = status;
            size_t reply_len = trashship__server_message__get_packed_size(&reply_msg);
            uint8_t reply_buf[256];
            trashship__server_message__pack(&reply_msg, reply_buf);
            comm_server_send(comm, reply_buf, reply_len);

            trashship__client_message__free_unpacked(msg, NULL);
        }
    }
    comm_close(comm);
    destroy_universe(rend, win);
    free(planets);
    free(trash);
    free(ship);
    return 0;
}

int read_keys(int initial_trash, int n_of_planets, struct planet_stucture planets[], struct trash_stucture trash[], SDL_Renderer* rend, struct trash_ship ship[], int universe_dimensions, int ship_capacity) {
    int pressed_key_code = 0;
    SDL_Event event;

    /* Process all pending events */
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                pressed_key_code = 1;
                break;
            default:
                break;
        }
    }

    return pressed_key_code;
}

//Function that allows objects to "wrap around" the universe borders
void correct_position(float *coord, int universe_dimensions) 
{
    if (*coord < 0) *coord += universe_dimensions;
    if (*coord >= universe_dimensions) *coord -= universe_dimensions;
}

int find_ship_index_by_id(struct trash_ship ship[], int n, int id)
{
    for (int i = 0; i < n; i++) 
    {
        if (ship[i].ID == id) 
        {
            return i;
        }
    }
    return -1;
}

int allocate_ship_slot(struct trash_ship ship[], int n, int universe_dimensions)
{
    int free_idx = -1;
    for (int i = 0; i < n; i++) 
    {
        if (ship[i].ID == 0) 
        {
            free_idx = i;
            break;
        }
    }
    if (free_idx < 0) return -1;

    int new_id = 0;
    int attempts = 0;

    do {
        new_id = (rand() % 1000000) + 1; 
        attempts++;
    } while (find_ship_index_by_id(ship, n, new_id) >= 0 && attempts < 16);

    if (find_ship_index_by_id(ship, n, new_id) >= 0) {
        return -1; 
    }

    ship[free_idx].ID = new_id;
    return free_idx;
}