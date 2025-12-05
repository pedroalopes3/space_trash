#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL2/SDL_pixels.h"   

#include "Communication.h"
#include "messages.pb-c.h"


int read_keys();   


int main(void)       
{

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    CommHandle *comm = comm_client_init();
    if (!comm) {
        fprintf(stderr, "Failed to init client communication\n");
        return -1;
    }


    // Create SDL window - THIS IS REQUIRED for keyboard input
   SDL_Window* window = SDL_CreateWindow("Universe Client", // creates a window
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       600, 600, 0);

    if (!window) {
        printf("error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

        // triggers the program that controls
    // your graphics hardware and sets flags
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;

    SDL_Renderer* rend = SDL_CreateRenderer(window, -1, render_flags);

    SDL_Color backgroud_color;
    backgroud_color.r = 255;
    backgroud_color.g = 255;
    backgroud_color.b = 255;
    backgroud_color.a = 255;
    SDL_SetRenderDrawColor(rend, 
        backgroud_color.r, backgroud_color.g, backgroud_color.b, 
        backgroud_color.a);
    SDL_RenderClear(rend);
    SDL_RenderPresent(rend);

    int close = 0;
    int key_pressed = 0;
    uint32_t client_id = 0;

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

        //Second: Receive ServerMessage with assigned id
        uint8_t reply_buf[256];
        int n = comm_client_recv(comm, reply_buf, sizeof(reply_buf));
        if (n > 0) 
        {
            Trashship__ServerMessage *reply = trashship__server_message__unpack(NULL, n, reply_buf);
            if (reply) 
            {
                client_id = reply->ship_id;
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

    while (!close) {
        key_pressed = read_keys();
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
            close = 1;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}


// Read keyboard input
int read_keys(){
    int pressed_key_code = 0;
    SDL_Event event;

    SDL_PollEvent(&event);

    switch (event.type) {
        case SDL_QUIT: 
            pressed_key_code = 1;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_UP:
                    pressed_key_code = 2;
                    break;
                case SDL_SCANCODE_LEFT:
                    pressed_key_code = 3;
                    break;
                case SDL_SCANCODE_DOWN:
                    pressed_key_code = 4;
                    break;
                case SDL_SCANCODE_RIGHT:
                    pressed_key_code = 5;
                    break;
                default:
                    break;
            }
    }
     
    return pressed_key_code;
}