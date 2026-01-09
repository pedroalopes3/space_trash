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

    //Load the variables that will be used by the threads
    ClientContext *ctx = malloc(sizeof(ClientContext));
    ctx->planets = planets;
    ctx->n_planets = n_of_planets;
    ctx->trash = trash;
    ctx->max_trash = max_trash;
    ctx->ship = ship;
    ctx->n_ships = n_of_planets;
    ctx->universe_dimensions = universe_dimensions;
    ctx->stop_flag = 0;

    if (pthread_mutex_init(&ctx->mutex, NULL) != 0)
    {
        fprintf(stderr, "Failed to init mutex\n");
        destroy_universe(rend, win);
        free(planets);
        free(trash);
        free(ship);
        free(ctx);
        return 1;
    }


    CommHandle *comm = comm_client_init();
    if (!comm) {
        fprintf(stderr, "Failed to init client communication\n");
        return -1;
    }
    
    int close = 0;
    int key_pressed = 0;
    int last_key = 0;
    uint32_t client_id = 0;

    SDL_TimerID timer_id = 0;
    timer_id = SDL_AddTimer(33,(SDL_TimerCallback)timer_callback, NULL);

    //First: Send a ClientMessage with ship_id = 0
    
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

        //Second: Receive a ServerMessage with an assigned id and an initial universe state
        uint8_t reply_buf[65536];  
        int n = comm_client_recv(comm, reply_buf, sizeof(reply_buf));
        if (n <= 0) 
        {
            fprintf(stderr, "Error: Server is not responding or has closed\n");
            fprintf(stderr, "Please ensure the server is running and try again.\n");
            destroy_universe(rend, win);
            free(planets);
            free(trash);
            free(ship);
            free(ctx);
            comm_close(comm);
            return 1;
        }
        
        Trashship__ServerMessage *reply = trashship__server_message__unpack(NULL, n, reply_buf);
        if (reply) 
        {
            // Check for error status from server
            if (reply->has_status && reply->status < 0)
            {
                if (reply->status == -2)
                {
                    fprintf(stderr, "Error: Server has no available ship slots\n");
                    fprintf(stderr, "The universe is full. Please try again later.\n");
                }
                else
                {
                    fprintf(stderr, "Error: Server returned error status %d\n", reply->status);
                }
                trashship__server_message__free_unpacked(reply, NULL);
                destroy_universe(rend, win);
                free(planets);
                free(trash);
                free(ship);
                free(ctx);
                comm_close(comm);
                return 1;
            }
            
            client_id = reply->ship_id;
            
            if (reply->initial_setup) 
            {
                Trashship__InitialSetupMessage *init_msg = reply->initial_setup;
                
                //store the information sent from the initial setup message into the local structures
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
            fprintf(stderr, "Error: Failed to parse server response\n");
            fprintf(stderr, "Server may be incompatible or not running properly.\n");
            destroy_universe(rend, win);
            free(planets);
            free(trash);
            free(ship);
            free(ctx);
            comm_close(comm);
            return 1;
        }


    pthread_t receive_thread;

    //create the thread that will handle the PUB/SUB socket communication
    pthread_create(&receive_thread, NULL, receive_universe_updates, (void*)ctx);

    while (!close) 
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        
        if (ctx->stop_flag) {
            close = 1;
            break;
        }
        
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
                key_pressed = 0;
                break;

            case SDL_USEREVENT:
            {
                if (event.user.code == 2) 
                {
                    //Draw universe every 33ms
                    pthread_mutex_lock(&ctx->mutex);
                    draw_universe(max_trash, n_of_planets, planets, trash, rend, ship);
                    pthread_mutex_unlock(&ctx->mutex);
                }
                break;
            }
            default:
                break;
        }
       
        
  
        if (key_pressed != last_key){
            // The ClientMessage is sent with both the command and the ID; since the ID was randomly calculated and assigned by 
            //server, clients will have a very difficult time cheating by pretending to be another ship.
            //a new command is only sent if it is different from the previous one: if a user holds down an input key there will only be sent a single initial
            //message with that command and the next message is only sent when the user lets go of the key.
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
            if (n <= 0) 
            {
                fprintf(stderr, "\nError: Lost connection to server\n");
                fprintf(stderr, "The server may have closed. Shutting down...\n");
                close = 1;
                break;
            }
            
            Trashship__ServerMessage *reply = trashship__server_message__unpack(NULL, n, reply_buf);
            if (reply) 
            {
                if (reply->has_status && reply->status != 0) 
                {
                    fprintf(stderr, "\nWarning: Server returned error status: %d\n", reply->status);
                    if (reply->status == -1)
                    {
                        fprintf(stderr, "Ship ID not found on server. Disconnecting...\n");
                        close = 1;
                    }
                }
                trashship__server_message__free_unpacked(reply, NULL);
            }
            else
            {
                fprintf(stderr, "\nError: Failed to parse server response\n");
                close = 1;
            }
            last_key = key_pressed;
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

    pthread_mutex_destroy(&ctx->mutex);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
