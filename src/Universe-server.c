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

   CommHandle *comm = comm_server_init();
    if (!comm) {
        fprintf(stderr, "Failed to init server communication\n");
        return -1;
    }


    // Create SDL window - THIS IS REQUIRED for keyboard input
   SDL_Window* window = SDL_CreateWindow("Universe Server", // creates a window
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       1000, 1000, 0);

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
        }
       
        char buffer[128];

        // Receive request
        int n = comm_server_recv(comm, buffer, sizeof(buffer)-1);
            if (n <= 0) {
                continue; 
            }

            buffer[n] = '\0';
            printf("Server request: %s\n", buffer);

            //Send reply
            const char *reply = "thanks!!!!";
            comm_server_send(comm, reply, strlen(reply));

            
        
             if (key_pressed == 1){
                close = 1;
             }
            sleep(0.1);
        }
    comm_close(comm);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

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
            
    }
     
    return pressed_key_code;
}
