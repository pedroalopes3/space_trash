#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <libconfig.h>
#include <SDL2/SDL_image.h>
#include "SDL2/SDL_pixels.h"
#include <pthread.h>   

#include "communication.h"
#include "messages.pb-c.h"
#include "display.h"
#include "universe-data.h"

typedef struct {
    struct planet_stucture *planets;
    int n_planets;
    struct trash_stucture *trash;
    int max_trash;
    struct trash_ship *ship;
    int n_ships;
    int universe_dimensions;
} ClientContext;


void* receive_universe_updates(void* arg);   

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

    //Allocate space for the planet and trash structures
    struct planet_stucture *planets = malloc(sizeof(*planets) * n_of_planets);
    struct trash_stucture *trash = malloc(sizeof(*trash) * max_trash);
    struct trash_ship *ship = malloc(sizeof(*ship)*n_of_planets);
    if (!planets || !trash || !ship) 
    {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    SDL_Window *win = NULL;
    SDL_Renderer *rend = NULL;
    client_display_init(&win, &rend, universe_dimensions);
    if (!win || !rend) 
    {
        fprintf(stderr, "Failed to create window/renderer\n");
        free(planets);
        free(trash);
        free(ship);
        return 1;
    }

    ClientContext *ctx = malloc(sizeof(ClientContext));
    ctx->planets = planets;
    ctx->n_planets = n_of_planets;
    ctx->trash = trash;
    ctx->max_trash = max_trash;
    ctx->ship = ship;
    ctx->n_ships = n_of_planets;
    ctx->universe_dimensions = universe_dimensions;


    CommHandle *comm = comm_client_init();
    if (!comm) {
        fprintf(stderr, "Failed to init client communication\n");
        return -1;
    }
    
    int close = 0;
    int key_pressed = 0;
    uint32_t client_id = 0;

    SDL_TimerID timer_id = 0;
    timer_id = SDL_AddTimer(33,(SDL_TimerCallback)timer_callback, NULL);

    //First: Send ClientMessage with ship_id = 0
    
        Trashship__ClientMessage msg = TRASHSHIP__CLIENT_MESSAGE__INIT;
        msg.has_ship_id = 1;
        msg.ship_id = 0;   
        msg.has_command = 1;
        msg.command = 0;
        size_t msg_len = trashship__client_message__get_packed_size(&msg);
        if (msg_len == 0) 
        {
            fprintf(stderr, "Error: ClientMessage packed to 0 bytes\n");
            return -1;
        }
        uint8_t msg_buf[256];
        trashship__client_message__pack(&msg, msg_buf);
        comm_client_send(comm, msg_buf, msg_len);

        //Second: Receive ServerMessage with assigned id and initial universe state
        uint8_t reply_buf[65536];  
        int n = comm_client_recv(comm, reply_buf, sizeof(reply_buf));
        if (n > 0) 
        {
            Trashship__ServerMessage *reply = trashship__server_message__unpack(NULL, n, reply_buf);
            if (reply) 
            {
                client_id = reply->ship_id;
                
                if (reply->initial_setup) 
                {
                    Trashship__InitialSetupMessage *init_msg = reply->initial_setup;
                    
                   
                    for (size_t i = 0; i < init_msg->n_planets && i < n_of_planets; i++) {
                        planets[i].x = init_msg->planets[i]->x;
                        planets[i].y = init_msg->planets[i]->y;
                        planets[i].isrecycle = init_msg->planets[i]->isrecycle;
                    }
                    
                    
                    for (int i = 0; i < max_trash; i++) {
                        trash[i].status = 0;
                    }
                    for (size_t i = 0; i < init_msg->n_trash && i < max_trash; i++) {
                        trash[i].x = init_msg->trash[i]->x;
                        trash[i].y = init_msg->trash[i]->y;
                        trash[i].status = init_msg->trash[i]->status;
                    }
                    
                    
                    for (size_t i = 0; i < init_msg->n_ships && i < n_of_planets; i++) {
                        ship[i].x = init_msg->ships[i]->x;
                        ship[i].y = init_msg->ships[i]->y;
                        ship[i].ID = init_msg->ships[i]->id;
                    }
                    
                }
                
                trashship__server_message__free_unpacked(reply, NULL);
            } 
            else 
            {
                fprintf(stderr, "Failed to unpack server message\n");
            }
        } 
        else 
        {
            printf("Failed to get ship id from server (n=%d)\n", n);
        }


    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_universe_updates, (void*)ctx);

    while (!close) 
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        switch (event.type) 
        {
            case SDL_QUIT: 
                key_pressed = 1;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode) 
                {
                    case SDL_SCANCODE_UP:
                        key_pressed = 2;
                        break;
                    case SDL_SCANCODE_LEFT:
                        key_pressed = 3;
                        break;
                    case SDL_SCANCODE_DOWN:
                        key_pressed = 4;
                        break;
                    case SDL_SCANCODE_RIGHT:
                        key_pressed = 5;
                        break;
                    default:
                        break;
                }
                break;

            case SDL_KEYUP:
                // Se largou a tecla para de enviar comando
                key_pressed = 0;
                break;

            case SDL_USEREVENT:
            {
                if (event.user.code == 2) 
                {
                    draw_universe(max_trash, n_of_planets, planets, trash, rend, ship);
                }
                break;
            }
            default:
                break;
        }
       
        
  
        if (key_pressed != 0){ 
            // Now the ClientMessage is sent with both the command and the ID; since the ID was randomly calculated and assigned by 
            //server, clients will have a very difficult time cheating by pretending to be another ship.
            Trashship__ClientMessage msg = TRASHSHIP__CLIENT_MESSAGE__INIT;
            msg.has_ship_id = 1;
            msg.ship_id = client_id;
            msg.has_command = 1;
            msg.command = (uint32_t)key_pressed;
            size_t msg_len = trashship__client_message__get_packed_size(&msg);
            uint8_t msg_buf[256];
            trashship__client_message__pack(&msg, msg_buf);
            comm_client_send(comm, msg_buf, msg_len);

            // Receive ServerMessage reply
            uint8_t reply_buf[256];
            int n = comm_client_recv(comm, reply_buf, sizeof(reply_buf));
            if (n > 0) 
            {
                Trashship__ServerMessage *reply = trashship__server_message__unpack(NULL, n, reply_buf);
                if (reply) 
                {
                    if (reply->status != 0) 
                    {
                        printf("Server error status: %d\n", reply->status);
                    }
                    trashship__server_message__free_unpacked(reply, NULL);
                }
            }
        }

        if (key_pressed == 1)
        {
            Trashship__ClientMessage msg = TRASHSHIP__CLIENT_MESSAGE__INIT;
            msg.has_ship_id = 1;
            msg.ship_id = client_id;
            msg.has_command = 1;
            msg.command = (uint32_t)key_pressed;
            size_t msg_len = trashship__client_message__get_packed_size(&msg);
            uint8_t msg_buf[256];
            trashship__client_message__pack(&msg, msg_buf);
            comm_client_send(comm, msg_buf, msg_len);
            close = 1;
        }

    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

void* receive_universe_updates(void* arg) 
{
    ClientContext *ctx = (ClientContext*)arg;
    int n_planets = ctx->n_planets;
    int universe_dimensions = ctx->universe_dimensions;
    struct trash_ship* ship = ctx->ship;
    struct planet_stucture* planets = ctx->planets;
    struct trash_stucture* trash = ctx->trash;
    int max_trash = ctx->max_trash;
    
    
    CommHandle *sub = comm_subscriber_init();
    if (!sub) {
        fprintf(stderr, "Failed to init subscriber\n");
        return NULL;
    }
    
    uint8_t buffer[65536];
    
    while (1) 
    {
        int n = comm_subscriber_recv(sub, buffer, sizeof(buffer));
        if (n <= 0) {
            continue;
        }
        
        Trashship__UpdateMessage *update_msg = trashship__update_message__unpack(NULL, n, buffer);

        if (update_msg) 
        {
                
            for (size_t i = 0; i < update_msg->n_planets; i++) 
            {
                int idx = update_msg->planets[i]->planet_index;
                if (idx >= 0 && idx < ctx->n_planets) 
                {
                    ctx->planets[idx].isrecycle = update_msg->planets[i]->isrecycle;
                }
            }
                
                
            for (int i = 0; i < ctx->max_trash; i++) {
                ctx->trash[i].status = 0; 
            }
            for (size_t i = 0; i < update_msg->n_trash && i < ctx->max_trash; i++) 
            {
                ctx->trash[i].x = update_msg->trash[i]->x;
                ctx->trash[i].y = update_msg->trash[i]->y;
                ctx->trash[i].status = update_msg->trash[i]->status;
            }
                
                // Process ships
            for (size_t i = 0; i < update_msg->n_ships && i < ctx->n_ships; i++) 
            {
                ctx->ship[i].x = update_msg->ships[i]->x;
                ctx->ship[i].y = update_msg->ships[i]->y;
                ctx->ship[i].ID = update_msg->ships[i]->id;
            }
                
            trashship__update_message__free_unpacked(update_msg, NULL);
        }
    }
}
    
