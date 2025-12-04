// Libraries
#include <stdio.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL2/SDL_pixels.h"   

#include "Communication.h"


// Function Declaration
int read_keys();   

// Main Function
int main(void)       
{

    // Initialize SDL
    //
    // returns zero on success else non-zero
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize communication with server

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
    printf("antes\n");
    while (!close) {
        key_pressed = read_keys();
        if (key_pressed != 0){ 
            printf("Key pressed code: %d\n", key_pressed);

            // Send Key to server
            char msg[8];
            snprintf(msg, sizeof(msg), "%d", key_pressed);
            comm_client_send(comm,msg, sizeof(msg)+1);

            // Server Reply
            char reply[100];
            int n = comm_client_recv(comm, reply, sizeof(reply));
            if (n > 0) {
                reply[n] = '\0';
                printf("Server: %s\n", reply);
            }

        }
        if (key_pressed == 1)
        {
            close = 1;
        }
        sleep(0.1);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}


// read keys from keyboard

int read_keys(){
    int pressed_key_code = 0;
    SDL_Event event;

    // Events management
    SDL_PollEvent(&event);

    switch (event.type) {
        case SDL_QUIT: 
            // handling of close button
            pressed_key_code = 1;
            break;

        case SDL_KEYDOWN:
            // keyboard API for key pressed
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